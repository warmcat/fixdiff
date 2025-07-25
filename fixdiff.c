/*
 * fixdiff
 *
 * Copyright (C) 2025 Andy Green <andy@warmcat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * See ./README.md for build and usage information.
 *
 */
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WIN32)
#include <unistd.h>
#define TO_POSLEN(x) (x)
#define OFLAGS(x) (x)
#else
#include <windows.h>
#include <processthreadsapi.h>
#include <BaseTsd.h>
#include <io.h>
#include <direct.h>
#define ssize_t SSIZE_T
#define open _open
#define read _read
#define lseek _lseek
#define close _close
#define write _write
#define getpid GetCurrentProcessId
#define chdir _chdir
#define unlink _unlink
typedef unsigned long pid_t;
#define TO_POSLEN(x) (unsigned int)(x)
#define OFLAGS(x) (_O_BINARY | (x))
#endif
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

typedef enum {
	DSS_WAIT_MMM,
	DSS_MUST_PPP,
	DSS_MUST_AA,
	DSS_AA_OR_MMM,
	DSS_PMSAD,
} dss_t;

/*
 * Readahead buffer for efficient lines from a file
 */

typedef struct {
	char		buf[4096];
	const char	*name;
	off_t		ro;
	size_t		bpos;
	size_t		blen;
	size_t		bls;
	int		fd;
	int		li;
} lbuf_t;

typedef struct {
	off_t		flo;

	const char	*reason;

	dss_t		d;
	int		pre;
	int		post;
	int		delta;
	int		lead_in;
	int		lead_out;
	int		lead_in_corrected;

	int		stanzas;
	int		bad;

	int		fd_temp;

	char		ongoing;
	char		skip_this_one;
	char		lead_in_active;
	char		cx_active;

	char		osh[128];
	char		temp[64];
	char		pf[512];

	lbuf_t		lb;
} dp_t;

static dp_t dp;

static void
init_lbuf(lbuf_t *plb, const char *name)
{
	plb->li = 0;
	plb->bpos = 0;
	plb->blen = 0;
	plb->ro = 0;
	plb->name = name;
}

#if 0

static void
hexdump(void *start, size_t len)
{
	static const char *hexchar = "0123456789abcdef";
	int n = 0, a = 0, used = 0, us = 0;
	uint8_t *p = (uint8_t *)start;
	char str[50], asc[17];

	memset(str, ' ', 48);
	memset(asc, ' ', 16);
	str[48] = '\0';
	asc[16] = '\0';

	while (len) {
		len--;
		if (a == 16) {
			fprintf(stderr, "%04X: %s  %s\n", us, str, asc);
			memset(str, ' ', 48);
			memset(asc, ' ', 16);
			a = 0;
			us = used;
		}

		str[a * 3] = hexchar[(*p) >> 4];
		str[(a * 3) + 1] = hexchar[(*p) & 15];

		if (*p < 32)
			asc[a] = '.';
		else
			asc[a] = (char)*p;

		a++;
		p++;
		used++;
	}
	if (a)
		fprintf(stderr, "%04X: %s  %s\n", us, str, asc);
}

#endif

/*
 * It's strcmp, but it is smart about matching a mixure of line endings
 */

typedef enum {
	LE_ZERO,
	LE_0A,
	LE_0D0A
} line_ending_t;

static line_ending_t
fixdiff_assess_eol(const char *a, size_t alen)
{
	if (alen >= 2 && a[alen - 2] == 0x0d && a[alen - 1] == 0x0a)
		return LE_0D0A;

	if (alen >= 1 && a[alen - 1] == 0x0a)
		return LE_0A;

	return LE_ZERO;
}

static int
fixdiff_strcmp(const char *a, size_t alen, line_ending_t *lea, const char *b,
	       size_t blen, line_ending_t *leb)
{
	*lea = fixdiff_assess_eol(a, alen);
	*leb = fixdiff_assess_eol(b, blen);

	if (alen - *lea != blen - *leb)
		/* accounting for EOL type, size mismatch */
		return 1;

	if ((*lea == LE_ZERO && *leb != LE_ZERO) ||
	    (*lea != LE_ZERO && *leb == LE_ZERO))
		/* one (not both) thought we ended without any CR or CRLF */
		return 1;

	if (alen - *lea == 0)
		/* both agree there is only some kind of CR / CRLF */
		return 0;

	return memcmp(a, b, alen - *lea);
}

static size_t
fixdiff_get_line(lbuf_t *plb, char *buf, size_t len)
{
	size_t l = 0;

	plb->bls = plb->ro + plb->bpos;

	while (len > 2) {

		if (plb->bpos == plb->blen) {
			ssize_t r;

			plb->ro = lseek(plb->fd, 0, SEEK_CUR);
			r = read(plb->fd, &plb->buf, sizeof(plb->buf));

			if (r <= 0) {
				if (l) {
					*buf++ = '\n';
					l++;
				}
				break;
			}


			plb->bpos = 0;
			plb->blen = r;
		}

		*buf++ = plb->buf[plb->bpos++];
		len--;
		l++;

		if (buf[-1] == '\n')
			/* we just reached the EOL */
			break;
	}

	*buf = '\0';
	plb->li++;

	return l;
}

static int
_mkstemp(const char *base, char *tmp, size_t len)
{
	pid_t pi = getpid();
	int fd;

	/* this cannot exceed the size of pdp->temp with given args */
	(void)snprintf(tmp, len - 1, "%s%lu", base, (unsigned long)pi);

	fd = open(tmp, OFLAGS(O_CREAT | O_TRUNC | O_RDWR), 0600);

	return fd;
}

static int
fixdiff_stanza_start(dp_t *pdp, char *sh, size_t len)
{
	pdp->pre		= 0;
	pdp->post		= 0;
	pdp->lead_in		= 0;
	pdp->lead_in_active	= 1;
	pdp->lead_out		= 0;
	pdp->cx_active		= 1;
	pdp->lead_in_corrected	= 0;
	pdp->d			= DSS_PMSAD;
	pdp->ongoing		= 1;

	pdp->stanzas++;

	if (len > sizeof(pdp->osh) - 1)
		len = sizeof(pdp->osh) - 1;

	memcpy(pdp->osh, sh, len);

	pdp->osh[len] = '\0';
	pdp->osh[sizeof(pdp->osh) - 1] = '\0';

	/*
	 * While in the stanza, we will issue stdin to a temp side-buffer.
	 *
	 * At the end of the stanza, we will issue a corrected header and then
	 * dump the side-buffer into stdout.  This way, we handle stdin in a
	 * single pass and don't care if the length of the header string was
	 * changed from the original.
	 */

	pdp->fd_temp = _mkstemp(".fixdiff", pdp->temp, sizeof(pdp->temp) - 1);
	if (pdp->fd_temp < 0) {
		fprintf(stderr, "Unable to create temp file (%d)", errno);
		return 1;
	}
	pdp->skip_this_one = 1;

	return 0;
}

static int
fixdiff_find_original(dp_t *pdp, int *line_start)
{
	char in_src[4096], in_temp[4096], hit = 0;
	lbuf_t lb_temp, lb_src, lb;
	size_t lt, ls;
	int ret = 1;

	/*
	 * We need to confirm the correct place in the file with the unchanged
	 * version.  Let's match ' ' and '-' lines, and skip '+' lines.
	 */

	lb_src.fd = lb.fd = -1;

	init_lbuf(&lb_temp, "temp");
	lb_temp.fd = open(pdp->temp, OFLAGS(O_RDWR));

	/*
	 * The idea is to set the starting point in the temp stanza for
	 * comparison in order to lose any extra lead_in
	 * (4 randomly seen with Gemini 2.5 where most are 3)
	 */

	while (pdp->lead_in > 3) {
		lt = fixdiff_get_line(&lb_temp, in_temp, sizeof(in_temp));
		if (!lt) {
			fprintf(stderr, "Unable to skip temp lines\n");
			return 1;
		}
		fprintf(stderr, "Stanza %d: removing extra lead-in\n", pdp->stanzas);
		pdp->lead_in--;
		pdp->lead_in_corrected++;
		pdp->pre--;
		pdp->post--;
	}

	/* the correct starting point in the temp stanza file */
	pdp->flo = (off_t)(lb_temp.ro + lb_temp.bpos);

	init_lbuf(&lb_src, "src");
	lb_src.fd = open(pdp->pf, OFLAGS(O_RDONLY));
	if (lb_src.fd < 0) {
		fprintf(stderr, "%s: Unable to open: %s: %d\n",
			__func__, pdp->pf, errno);
		close(lb_temp.fd);
		return 1;
	}

	/*
	 * Outer loop walks through each line in source.
	 * Inner loop tries to match starting from that line
	 */

	while (!hit) {
		line_ending_t let, les;

		init_lbuf(&lb, "src_comp");
		lb.fd = open(pdp->pf, OFLAGS(O_RDONLY));
		lb.li = lb_src.li;
		lseek(lb.fd, (off_t)(lb_src.ro + lb_src.bpos), SEEK_SET);

		init_lbuf(&lb_temp, "lb_temp");
		lseek(lb_temp.fd, pdp->flo, SEEK_SET);

		while (!hit) {
			ls = fixdiff_get_line(&lb, in_src, sizeof(in_src));
			if (!ls) {
				fprintf(stderr, "failed on src exhaustion\n");
				break;
			}

			do {
				lt = fixdiff_get_line(&lb_temp, in_temp, sizeof(in_temp));
				if (!lt) { /* ran out of stanza before mismatch / EOF */
					hit = 1;
					break;
				}

			} while (in_temp[0] == '+');

			if (hit)
				break;
			if (fixdiff_strcmp(in_temp + 1, lt - 1, &let, in_src, ls, &les))
				break;
		}

		close(lb.fd);

		/* move forward */
		lt = fixdiff_get_line(&lb_src, in_src, sizeof(in_src));
		if (!lt) /* ie, return failed */
			goto out;
	}

	if (hit) {
		ret = 0;
		*line_start = lb_src.li;

		if (pdp->cx_active < 3) {
			lbuf_t lb_ef;
			int a = 0;

			init_lbuf(&lb_ef, "end_fill");
			lb_ef.fd = open(pdp->pf, OFLAGS(O_RDONLY));
			lb_ef.li = lb.li;
			lseek(lb_ef.fd, (off_t)lb.bls, SEEK_SET);

			/*
			 * Suspected patch at EOF
			 *
			 * It's fine if we can't add anything at end, it
			 * means it was already correct.  Otherwise there are
			 * actual lines in the sources that must be added to
			 * the stanza as ' ' lines.
			 */

			while (pdp->cx_active < 3) {
				line_ending_t lea;

				ls = fixdiff_get_line(&lb_ef, in_src + 1, sizeof(in_src) - 1);
				if (!ls)
					break;

				lea = fixdiff_assess_eol(in_src + 1, ls);

				in_src[0] = ' ';
				lseek(lb_temp.fd, 0, SEEK_END);
				if (write(lb_temp.fd, in_src, TO_POSLEN(ls + 1 - lea)) !=
					  (ssize_t)(ls + 1 - lea)) {
					close(lb_ef.fd);
					pdp->reason = "failed to write extra stanza"
							"trailer to temp file";
					ret = 1;
					goto out;
				}

				if (lea != LE_ZERO)
					if (write(lb_temp.fd, "\n", TO_POSLEN(1)) !=
						  (ssize_t)1) {
						close(lb_ef.fd);
						pdp->reason = "failed to write extra "
								"stanza trailer to temp file";
						ret = 1;
						goto out;
					}

				pdp->pre++;
				pdp->post++;
				pdp->cx_active++;
				a++;
			}

			if (a)
				fprintf(stderr, "Stanza %d: detected patch at EOF: "
						  "added %d context at end\n",
					pdp->stanzas, a);

			close(lb_ef.fd);
		}
	}

out:
	close(lb_temp.fd);
	close(lb_src.fd);

	return ret;
}

static int
fixdiff_stanza_end(dp_t *pdp)
{
	char buf[256];
	int orig;

	if (!pdp->ongoing)
		return 0;

	if (fixdiff_find_original(pdp, &orig)) {
		fprintf(stderr, "Unable to find original stanza in source\n");
		goto probs;
	}

	/* let's create a stanza header with our computed numbers in */

	if (strlen(pdp->osh) < 8 ||
	    pdp->osh[0] != '@' ||
	    pdp->osh[1] != '@' ||
	    pdp->osh[2] != ' ' ||
	    pdp->osh[3] != '-')
		goto probs;

	/* record length of lead-out context */
	pdp->lead_out = pdp->cx_active;

	/* We don't use anything from the original header. */

	snprintf(buf, sizeof(buf) - 1, "@@ -%d,%d +%d,%d @@\n",
		 orig, pdp->pre, orig + pdp->delta, pdp->post);

	/* is that what we already had? */

	if (strcmp(buf, pdp->osh)) {
		fprintf(stderr, "  - (lead_in %d, lead_out %d) %s", pdp->lead_in, pdp->lead_out, buf);
		pdp->bad++;
	}

	if (write(1, buf, TO_POSLEN(strlen(buf))) != (ssize_t)strlen(buf)) {
		pdp->reason = "failed to write stanza header to stdout";
		return 1;
	}

	/* dump the temp side-buffer into stdout */

	lseek(pdp->fd_temp, pdp->flo, SEEK_SET);
	while (1) {
		ssize_t l = read(pdp->fd_temp, buf, sizeof(buf));
		if (!l)
			break;

		if (write(1, buf, TO_POSLEN(l)) != (ssize_t)l) {
			pdp->reason = "failed to write to stdout";
			return 1;
		}
	}

	close(pdp->fd_temp);
	pdp->fd_temp = -1;

	/* track the effect stanza changes are having on line offsets */
	pdp->delta += pdp->post - pdp->pre;

	pdp->ongoing = 0;

	return 0;

probs:
	pdp->reason = "Original stanza format problem";

	return 1;
}

int
main(int argc, char *argv[])
{
	char in[4096];
	ssize_t w;

	(void)argc;
	(void)argv;

#if defined(WIN32)
	SetConsoleOutputCP(65001); /* utf8 */
	/*
	 * The problem is cat or type sending to stdin will have opened
	 * the file it is sending using _O_TEXT, so we have to match
	 */
	_setmode(0, _O_TEXT);
	_setmode(1, _O_BINARY);
#endif

	/* if there is a commandline arg, we cwd to it first */
	if (argc > 1)
		chdir(argv[1]);

	memset(&dp, 0, sizeof(dp));
	init_lbuf(&dp.lb, "stdin");
	dp.reason = "unknown";
	dp.d = DSS_WAIT_MMM;
	dp.lb.fd = 0; /* stdin */
	dp.fd_temp = -1;

	while (1) {
		size_t l = fixdiff_get_line(&dp.lb, in, sizeof(in));

		if (!l) {
			if (fixdiff_stanza_end(&dp))
				goto bail;
			break;
		}

		switch (dp.d) {
		case DSS_WAIT_MMM:
			if (l < 4)
				break;
			if (in[0] == '-' &&
			    in[1] == '-' &&
			    in[2] == '-' &&
			    in[3] == ' ')
				dp.d = DSS_MUST_PPP;
			break;

		case DSS_MUST_PPP:
			if (l < 4)
				break;
			if (in[0] == '+' &&
			    in[1] == '+' &&
			    in[2] == '+' &&
			    in[3] == ' ') {
				int n = 4, sl = 1;
				char *p;

				while (sl && in[n]) {
					while (in[n] && in[n] != '/')
						n++;
					if (!in[n])
						goto bail;
					n++;
					if (!in[n])
						goto bail;
					sl--;
				}

				strncpy(dp.pf, in + n, sizeof(dp.pf));
				dp.pf[sizeof(dp.pf) - 1] = '\0';
				p = strchr(dp.pf, '\n');
				if (p)
					*p = '\0';

				fprintf(stderr, "Filepath: %s\n", dp.pf);

				dp.d = DSS_MUST_AA;
				break;
			}

			dp.reason = "+++ required but not found";
			goto bail;

		case DSS_MUST_AA:
			if (l < 3) {
				dp.reason = "@@ required but line too short";
				goto bail;
			}
			if (in[0] == '@' &&
			    in[1] == '@' &&
			    in[2] == ' ') {
				if (fixdiff_stanza_start(&dp, in, l))
					goto bail;
				dp.d = DSS_PMSAD;
				break;
			}

			dp.reason = "@@ required but mssing";
			goto bail; /* MUST have been AA */

		case DSS_AA_OR_MMM:
			if (l < 4)
				break;
			if (in[0] == '-' &&
			    in[1] == '-' &&
			    in[2] == '-' &&
			    in[3] == ' ') {
				dp.d = DSS_MUST_PPP;
				break;
			}

			if (in[0] == '@' &&
			    in[1] == '@' &&
			    in[2] == ' ') {
				if (fixdiff_stanza_start(&dp, in, l))
					goto bail;
				break;
			}
			break;

		case DSS_PMSAD:
			if (l < 1) {
				dp.reason = "blank line in stanza";
				goto bail;
			}

			if (in[0] == ' ') { /* Space */
				dp.pre++;
				dp.post++;
				if (dp.lead_in_active)
					dp.lead_in++;
				dp.cx_active++;
				break;
			} else
				if (in[0] == '-') { /* Minus */
					dp.pre++;
					dp.lead_in_active = 0;
					dp.cx_active = 0;
					break;
				} else
					if (in[0] == '+') { /* Plus */
						dp.post++;
						dp.lead_in_active = 0;
						dp.cx_active = 0;
						break;
					}

			if (l > 5 &&
			    in[0] == 'd' &&
			    in[1] == 'i' &&
			    in[2] == 'f' &&
			    in[3] == 'f' &&
			    in[4] == ' ') { /* Diff */
				if (fixdiff_stanza_end(&dp))
					goto bail;
				dp.d = DSS_WAIT_MMM;
				break;
			}

			if (l > 3 &&
			    in[0] == '@' &&
			    in[1] == '@' &&
			    in[2] == ' ') { /* At */
				if (fixdiff_stanza_end(&dp))
					goto bail;
				if (fixdiff_stanza_start(&dp, in, l))
					goto bail;
				break;
			}
			fprintf(stderr, "'%c' (0x%x)\n", in[0], in[0]);
			dp.reason = "unexpected character in stanza";
			goto bail;
		}

		if (dp.skip_this_one) {
			dp.skip_this_one = 0;
			continue;
		}

		w = write(dp.fd_temp != -1 ? dp.fd_temp : 1, in, TO_POSLEN(l));
		if (w < 0) {
			fprintf(stderr, "write to stdout failed: %d\n", errno);
			goto bail;
		}
	}

	fprintf(stderr, "Completed: %d / %d stanza headers repaired\n",
		dp.bad, dp.stanzas);

	unlink(dp.temp);

	return 0;

bail:
	fprintf(stderr, "line %d: fatal exit: %s: %s\n", dp.lb.li, dp.reason, in);

	unlink(dp.temp);

	return 1;
}
