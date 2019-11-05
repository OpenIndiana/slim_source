/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 1992 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */


/*
 * mk_msgs:
 *
 *	Utility to automatically generate:
 *
 *		1) compilable C files defining arrays of
 *		   default message strings
 *
 *		2) header files defining indices into the message arrays
 *
 *		3) a portable object (.po) file that can be used by
 *		   a translator to do message localization
 *
 *
 *	Input consists of a formatted ASCII file that looks like this:
 *
 *		CFILE	<base name of generated .h & .c files>
 *		PREFIX	<unique prefix prepended to msg index references>
 *		BASE	basecode	basename
 *		msgref	code	"message string"
 *		"another message string for the above msgref"
 *			"	"	"	"
 *			"	"	"	"
 *	  where:
 *		CFILE is a keyword,  followed by a unique filename word.
 *			It is used for naming of the files and
 *			variables automatically	generated by the program.
 *
 *			There may be multiple CFILE's.  Each CFILE
 *			instantiates a new header file and a new C src
 *			containing the default text strings.  However,
 *			no new .po file is created - all text messages
 *			are assumed to belong in the same domain.
 *
 *		PREFIX is a keyword, followed by a unique prefix word.
 *			The prefix is used to create unique message
 *			identifiers composed of: <prefix>_<msgref>.
 *
 *			There may be multiple PREFIX's, and you can change
 *			the prefix at any time.
 *
 *		BASE is a keyword, followed by the starting msg index
 *			for this group of messages, and basename is a unique
 *			identifier for this group of messages.
 *
 *			There may be multiple base groupings within
 *			a group associated with a particular PREFIX.
 *			You must specify indicies that do not overlap
 *			another BASE range within the same PREFIX group.
 *
 *		msgref is the unique identifier a given message.
 *
 *		code is the message number, relative to basecode.
 *			A code of zero means just use the next default
 *			value in sequence.  Good for when defined constant
 *			values do not have to remain the same from one
 *			release to the next.
 *
 *		The first string for a message MUST be the 3rd argument
 *		on the msgref line.  Additional strings must appear on
 *		subsequent lines, and each string MUST be quoted (").
 *		Additional strings may be indented by white spaces ie.,
 *		the first quote may be indented for clarity sake, if
 *		desired.
 *
 *	Any line starting with a '#' in column 1 is treated as a comment.
 *	Blank lines are ignored.
 *
 *	The program will recognize 2 arguments:
 *		"-a" disables ANSI-style concatenation of strings in a
 *			a string vector.  Use this if your code will
 *			be running on SunOS 4.x.  The default is ANSI.
 *
 *		"-d <text domain>" specifies the name of the text
 *			domain your messages (.mo file) will reside.
 *
 *
 *	Here's a simple example that would generate these files:
 *		adm_error.h
 *		adm_error.c
 *		bar_msg.h
 *		bar_msg.c
 *		adm_messages.po
 *
 *
 * ####################################################
 * 
 * #	ADM error messages
 * CFILE	ADM_ERROR
 * PREFIX ADM
 * BASE 0	AMCL
 * SUCCESS		0	"Success"
 * GOOD		0	"GOOD"
 * BETTER		0	"BETTER"
 * BEST		0	"BEST"
 * 
 * PREFIX	ADM_ERR
 * FILE_NOT_FOUND	0	"File not found. "
 * 	"You don't have access you schmuck"
 * 
 * CANT_DO_THIS	8	"can't do this"
 * GETTYSBURG	0	"Four score and seven"
 * 	" years ago,\n our fathers brought forth"
 * 	" on this continent,\n a new nation,"
 * 	" conceived for Sun Microsystems,\n"
 * 	" that all UNIX operating systems should be the same."
 * 
 * HELLO		0	"Hello world"
 * 
 * BASE	15	AMSL
 * FOOBAR	0	"Foo Bar"
 * 
 * # ADM holiday messages
 * PREFIX	ADM_MSG
 * BASE 20	HAPPY
 * XMAS		0	"Merry Xmas"
 * NEW_YEAR	2	"Happy New Year"
 * 
 * 
 * 
 * # BAR messages
 * CFILE	BAR_MSG
 * PREFIX BAR
 * BASE	100	MSG
 * SUN_ADDRESS	1	"Sun Microsystems"
 * " 2 Federal Street"
 * " Billerica, MA"
 * 
 * ####################################################
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>

#define	MSGID		"msgid"		/* .po file keyword */
#define	MSGSTR		"msgstr"	/* .po file keyword */
#define	PREFIX		"PREFIX"	/* keyword */
#define	CFILE		"CFILE"		/* keyword */
#define	BASE		"BASE"		/* keyword */
#define	DEFAULT_PREFIX	"MESSAGE"	/* if PREFIX not defined */
#define	DEFAULT_BASE_NM	"MSG"		/* if basename not specified */
#define	DEFAULT_CFILE	"msg_msg"	/* if filename not specified */
#define	DEFAULT_DOMAIN	"msg_domain"	/* if not specified via "-d" argument */
#define	DEFAULT_BASE	0		/* if base not specified */

/*
 * Maximum lengths that should be more than enough.  Anybody
 * using longer lengths is being ridiculous.
 */
#define	MAX_LINE	1024	/* max length of input line */
#define	MAX_PREFIX	50
#define	MAX_FILE	50
#define	MAX_BASE	50
#define	MAX_CODENAME	50


#define	ERR_BASE_TOO_SMALL \
"ERROR: specified BASE value of %d is too low.  Use a value > %d.\n"

#define	ERR_CODE_TOO_SMALL \
"ERROR: code value of %d at line:\n\t%s\nis too low.  Use a value > %d.\n"

#define	ERR_MISSING_QUOTES \
"ERROR: Text string `%s' is not enclosed in quotes (\").\n"

#define	ERR_MISSING_TEXT \
"ERROR: Text field in the line `%s' is either missing or is not enclosed \
in quotes (\").\n"


#define	WARNING \
"/* * * * * * * * * * * * * * * * * * * * *\n\
 *\n\
 *	NOTE: This is NOT source code!!!\n\
 *\n\
 *	This file was generated by the program\n\
 *		%s\n\
 *	Don't bother editing this file because\n\
 *	your changes will be clobbered.\n\
 *\n\
 * * * * * * * * * * * * * * * * * * * * */\n\n"


#define	COPYRIGHT \
"/*\n\
 * Copyright 1991 Sun Microsystems, Inc.  All Rights Reserved.\n\
 * Use is subject to license terms.\n\
 */\n\n"


/*
 * Macro used to get a copy of the PREFIX variable, and
 * convert it to all lower-case characters, to be used for
 * C variable declarations.
 */
#define	LOWER_CASE(s) \
	for (i=0; s[i]; i++)	\
		if (isupper(s[i]))	\
			s[i] = tolower(s[i])


/* Standard header file ifndef-endif bracketing */
#define	HEADER_IFNDEF \
	fprintf(fp_header, "#ifndef\t_%s_H", cfile); \
	fprintf(fp_header, "\n#define\t_%s_H\n\n", cfile); \
	fprintf(fp_header, "#include <libintl.h>\n\n"); \
	fprintf(fp_header, "#define\t%s_TEXTDOMAIN\t\"%s\"\n\n", cfile, text_domain);

#define	HEADER_ENDIF \
	fprintf(fp_header, "\n#endif /* _%s_H */\n", cfile);



/* First and last codes for entire header file.  Encompasses all BASEs */
#define	FIRST_CODE \
	fprintf(fp_header, "#define\t%s_BASE\t%d\n\n", \
		cfile, base)

#define	LAST_CODE \
	fprintf(fp_header, "#define\t%s_LAST\t%d\n\n", \
		cfile, top_code)


#define	I18N_FUNCS \
"#ifndef __STDC__\n\
extern	char	*dgettext();\n\
extern	char	*gettext();\n\
extern	char	*textdomain();\n\
extern	int	bindtextdomain();\n#endif\n\n"

#define	DEFINE_EXTERNS \
	fprintf(fp_header, "typedef\tint\t%c%s;\n\n", \
		toupper(decl[0]), &decl[1]); \
	fprintf(fp_header, I18N_FUNCS); \
	fprintf(fp_header, "extern\tchar\t*%s[];\n\n", \
		decl);

#define	DEFINE_MACROS \
	fprintf(fp_header, "/* Useful macros */\n"); \
	fprintf(fp_header, "#define\t%s(i)\tdgettext(%s_TEXTDOMAIN,", \
		cfile, cfile); \
	fprintf(fp_header, " %s[i-%s_BASE])\n", \
		decl, cfile);



/* First and last codes for a particular BASE grouping */
#define	START_BASE \
	fprintf(fp_header, "#define\t%s_BASE_%s\t%d\n", \
		cfile, base_name, base)

#define	END_BASE \
	fprintf(fp_header, "#define\t%s_END_%s\t(%s_BASE_%s + %d)\n\n", \
		cfile, base_name, cfile, base_name, top_code-base)


/* New defined code constant */
#define	ADD_CODE \
	fprintf(fp_header, "#define\t%s_%s\t(%s_BASE_%s + %d)\n", \
		prefix, codename, cfile, base_name, code)




extern	int	getopt();
extern	char	*optarg;

static	void	open_file();
static	void	null_messages();


static	char	cfile[MAX_FILE+1];	/* user-specified value of CFILE */
static	char	pofile[MAX_FILE+1];	/* the text domain, via the "-d" option */
static	char	prefix[MAX_PREFIX+1];	/* user-specified value of PREFIX */
static	char	decl[MAX_PREFIX+1];	/* prefix, but all lower case, used */
					/* for variable declarations */

static	char	base_name[MAX_BASE];	/* user-specified value of basename */
static	char	*prog;			/* the name of this program */
static	int	base = DEFAULT_BASE;	/* assume default in case none */
					/* specified */

static	int	ansi = 1;		/* 0=make msg array compatible with non-ANSI 4.x C */
					/* 1=make msg array compatible with ANSI C */

static	int	first_msg;		/* 0 = next msg is not the first to */
					/*   to be output to msg array */
					/* 1 = next msg will be the first to */
					/*   to be output to msg array */

static	char	*text_domain = DEFAULT_DOMAIN;


int
main(argc, argv)
	int	argc;
	char	**argv;
{
	char	line[MAX_LINE];
	char	msg[MAX_LINE];
	char	codename[MAX_CODENAME];	/* name of code constant */
	FILE	*fp_text;	/* pointer to generated C file */
				/* containing msgid strings */

	FILE	*fp_header;	/* pointer to generated header file */
				/* containing constant indices into */
				/* msgid string vector */

	FILE	*fp_po;		/* pointer to generated .po file */
	int	i, c;
	int	po_flag = 0;	/* indicates if .po file written to yet */
	int	code;		/* code constant */
	int	top_code;	/* Highest defined constant within a BASE */
	char	*p, *q;

	/* Initialization, set up defaults */

	prog = argv[0];

	while ((c=getopt(argc, argv, "ad:")) != -1) {
		switch (c) {
		case 'a' :
			ansi--;
			break;

		case 'd' :
			text_domain = optarg;
			break;

		default:
			fprintf(stderr, "Usage: %s [-a] [-d <text domain>]\n", prog);
			exit(99);
			break;
		}
	}

	fp_header = NULL;
	fp_text = NULL;
	fp_po = NULL;
	top_code = DEFAULT_BASE - 1;
	strcpy(base_name, DEFAULT_BASE_NM);
	strcpy(pofile, text_domain);
	strcpy(cfile, DEFAULT_CFILE);
	strcpy(prefix, DEFAULT_PREFIX);
	strcpy(decl, cfile);
	LOWER_CASE(decl);

	while (fgets(line, sizeof(line), stdin) != NULL) {

		/*
		 * Check for comment or empty line, which are
		 * simply ignored.
		 */
		if ((line[0] == '#') || (line[0] == '\0'))
			continue;
		for (p=line; *p && isspace(*p); p++);
		if (!*p)
			continue;

		/*
		 * Check for PREFIX keyword.  Each occurrence of this
		 * causes the existing header and C files to be completed
		 * and closed.  The PREFIX keyword signals a program
		 * "reset", EXCEPT for the .po file.  It is presumed that
		 * all text messages should be in the same domain, and so
		 * additional text from multiple PREFIX segments is simply
		 * appended to the .po file.
		 */
		if (!strncmp(line, CFILE, strlen(CFILE))) {
			if (fp_header != NULL) {
				END_BASE;
				LAST_CODE;
				DEFINE_EXTERNS;
				DEFINE_MACROS;
				HEADER_ENDIF;
				fclose (fp_header);
				fp_header = NULL;
				base = 0;
				top_code = 0;
			}
			if (fp_text != NULL) {
				if (!ansi)
					putc('"', fp_text);
				fprintf(fp_text, "\n};\n");
				fclose (fp_text);
				fp_text = NULL;
			}

			/* Read the FILE value */
			sscanf(line + strlen(CFILE), "%s", cfile);
			strcpy(decl, cfile);
			LOWER_CASE(decl);
		}

		/* Check for PREFIX keyword */
		else if (!strncmp(line, PREFIX, strlen(PREFIX))) {
			sscanf(line + strlen(PREFIX), "%s", prefix);
		}


		/* Check for BASE keyword */
		else if (!strncmp(line, BASE, strlen(BASE))) {

			/*
			 * If we've already written to the header file,
			 * then we're creating a new BASE grouping, so
			 * finish off the previous group by defining the
			 * last msg code for that base group.
			 */
			if (fp_header != NULL) {
				END_BASE;
			}

			/* Read in new base code and base name */
			sscanf(line + strlen(BASE), "%d %s", &base, base_name);

			/* Fatal if base code overlaps a previous base range */
			if ((fp_header != NULL) && (base <= top_code+1)) {
				fprintf(stderr, ERR_BASE_TOO_SMALL, base,
					top_code);
				exit(3);
			}


			/*
			 * If header file not opened, then open it and
			 * initialize.
			 */
			if (fp_header == NULL) {
				open_file(&fp_header, decl, ".h");
				HEADER_IFNDEF;
				FIRST_CODE;
			}


			/*
			 * If base range has gaps, output empty message
			 * strings to message vector.
			 */
			null_messages(fp_text, top_code+1, base);

			/*
			 * Note the highest code used, and introduce
			 * defined constants for the new base group.
			 */
			top_code = base - 1;
			START_BASE;


		} else {

			/*
			 * Input consists of either a new message reference,
			 * or continuation of message strings for current
			 * message reference.  We'll be writing to all 3
			 * files, so make sure they're created and initialized.
			 * If we need to initialize the header file, then
			 * no BASE was specified, so we need to initialize
			 * the first msgref for this base grouping.
			 */
			open_file(&fp_text, decl, ".c");
			open_file(&fp_po, pofile, ".po");
			if (fp_header == NULL) {
				open_file(&fp_header, decl, ".h");
				HEADER_IFNDEF;
				FIRST_CODE;
				START_BASE;
			}


			/* Skip over white spaces. */
			for (p=line; *p && isspace(*p); p++);

			/* Check for quote (") as first non-blank character */
			if (*p == '"') {
				/*
				 * Message string continuation, so simply
				 * send the message to the message vector
				 * and the .po file.  But first check to
				 * make sure string is enclosed in quotes.
				 */
				if (p[strlen(p)-1] != '"') {
					fprintf(stderr, ERR_MISSING_QUOTES, p);
					exit(5);
				}
				strcpy(msg, p);
				if (ansi)
					fprintf(fp_text, "\n\t%s", p);
				else {
					/*
				 	 * non ANSI C.  Must strip off quotes and
				 	 * append to previous text.
					 */
					q = strdup(p);
					q[0] = '\0';
					p = &q[1];
					p[strlen(p)-1] = '\0';
					fprintf(fp_text, "%s", p);
					free(q);
				}
				fprintf(fp_po, "%s\n", msg);

			} else {

				/*
				 * New message reference, so read in
				 * codename, code, and first msg string.
				 */
				sscanf(line, "%s %d %[^\n]",
					codename, &code, msg);

				/*
				 * Code value of zero means code should
				 * default to the next value in sequence.
				 */
				if (code == 0)
					code = top_code - base + 1;


				/*
				 * Fatal if msg code overlaps a previous
				 * code within this base group.
				 */
				if ((base + code) <= top_code) {
					fprintf(stderr, ERR_CODE_TOO_SMALL,
						code, line, top_code-base);
					exit(4);
				}


				/* Define new constant in header file */
				ADD_CODE;

				/*
				 * Gaps mean we gotta output empty msg strings
				 * so the indices will work.
				 */
				null_messages(fp_text, top_code+1, base+code);

				/* Note highest code used */
				top_code = base + code;

				if ((msg[0] != '"') ||
				    (msg[strlen(msg)-1] != '"')) {
					fprintf(stderr, ERR_MISSING_TEXT,
						line);
					exit(5);
				}

				/*
				 * Now output the message string to the
				 * message vector and the .po file.  Write
				 * null msgstr to .po file for the previous
				 * msgid (if this is not the first).
				 */
				if (ansi) {
					if (!first_msg)
						fprintf(fp_text, ",");
					first_msg = 0;
					fprintf(fp_text, "\n\n/* %s_%s */\n\t%s",
						prefix, codename, msg);
				} else {
					/*
					 * non ANSI C.  Must strip off quotes and
					 * append to previous text.
					 */
					q = strdup(msg);
					q[0] = '\0';
					p = &q[1];
					p[strlen(p)-1] = '\0';
					if (!first_msg)
						fprintf(fp_text, "\",");
					first_msg = 0;
					fprintf(fp_text, "\n\n/* %s_%s */\n\t\"%s",
						prefix, codename, p);
					free(q);
					first_msg = 0;
				}
				if (po_flag)
					fprintf(fp_po, "%s\n#\n", MSGSTR);
				fprintf(fp_po, "%s %s\n", MSGID, msg);
				po_flag++;
			}
		}
	}

	/* Finish off each output file */
	if (!ansi)
		putc('"', fp_text);
	fprintf(fp_text, "\n};\n");

	END_BASE;
	LAST_CODE;
	DEFINE_EXTERNS;
	DEFINE_MACROS;
	HEADER_ENDIF;

	fprintf(fp_po, "%s\n", MSGSTR);

	fclose(fp_text);
	fclose(fp_header);
	fclose(fp_po);

	exit(0);
}




/*
 * open_file:
 *
 *	Compose a file name consisting of the fname, appended
 *	by type.  Do standard file initialization based on type.
 */

static void
#ifdef __STDC__
open_file(
	FILE	**fp,
	char	*fname,
	char	*type)

#else
open_file(fp, fname, type)
	FILE	**fp;
	char	*fname;
	char	*type;
#endif

{
	char	filename[MAXPATHLEN];

	if (*fp != NULL)
		return;

	sprintf(filename, "%s%s", fname, type);
	*fp = fopen(filename, "w");
	if (*fp == NULL) {
		fprintf(stderr, "ERROR: unable to open %s\n", filename);
		perror("Reason");
		exit(1);
	}

	if (!strcmp(type, ".h")) {
		fprintf(*fp, WARNING, prog);
		fprintf(*fp, COPYRIGHT);
	}

	if (!strcmp(type, ".c")) {
		fprintf(*fp, WARNING, prog);
		fprintf(*fp, COPYRIGHT);
		fprintf(*fp, "char\t*%s[] = {", decl);
		first_msg = 1;
	}

	if (!strcmp(type, ".po")) {
		fprintf(*fp, "domain %s\n", fname);
	}

}




/*
 * null_messages
 *
 *	Routine to output empty strings, for when there are gaps in
 *	the code sequence within a base, or between base groupings.
 */

static void
#ifdef __STDC__

null_messages(
	FILE	*fp_text,
	int	start,
	int	end)

#else

null_messages(fp_text, start, end)
	FILE	*fp_text;
	int	start;
	int	end;

#endif

{
	int	i;
#define	UNUSED	"\n/* UNUSED */\t\""

	if (fp_text == NULL)
		return;

	for (i=start; i<end; i++) {
		if (i == start) {
			if (!first_msg) {
				if (!ansi)
					putc('"', fp_text);
				fprintf(fp_text, ",");
			}
			first_msg = 0;
			fprintf(fp_text, "\n%s", UNUSED);
			if (ansi)
				putc('"', fp_text);
		} else {
			if (!ansi)
				putc('"', fp_text);
			fprintf(fp_text, ",%s", UNUSED);
			if (ansi)
				putc('"', fp_text);
		}
	}
}
