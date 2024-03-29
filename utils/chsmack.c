/*
 * chsmack - Set smack attributes on files
 *
 * Copyright (C) 2011 Nokia Corporation.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, version 2.
 *
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public
 *	License along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *	02110-1301 USA
 *
 * Author:
 *      Casey Schaufler <casey@schaufler-ca.com>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LSIZE 23

static inline int leads(char *in, char *lead)
{
	return (strncmp(in, lead, strlen(lead)) == 0);
}
/*
Label rules

Smack labels are ASCII character strings,
1. one to twenty-three characters in length.
2. cannot contain unprintable characters, the "/" (slash),
	 the "\" (backslash), the "'" (quote) and '"' (double-quote) characters.
3. cannot begin with a '-', which is reserved for special options.
*/

static int rules(const char *label)
{
	int idx, len, ch;

	idx = 0;
	len = strlen(label);

	if (len > LSIZE)
		return 1;
	else {
		if (label[idx] != '-') {
			while (idx < len) {
				ch = label[idx++];
				if (ch == 0x2F || ch == 0x5C || ch == 0x27 ||
					 ch == 0x2A || ch == 0x7F || ch < 0x1F)
					break;
			}
			if (idx < len)
				return 1;
			else
				return 0;
		}
		else
			return 1;
	}
}

int
main(int argc, char *argv[])
{
	int rc;
	int argi;
	int transmute = 0;
	char buffer[LSIZE + 1];
	char *access = NULL;
	char *mm = NULL;
	char *execute = NULL;

	for (argi = 1; argi < argc; argi++) {
		if (strcmp(argv[argi], "-a") == 0)
			access = argv[++argi];
		else if (leads(argv[argi], "--access="))
			access = argv[argi] + strlen("--access=");
		else if (strcmp(argv[argi], "-e") == 0)
			execute = argv[++argi];
		else if (leads(argv[argi], "--exec="))
			execute = argv[argi] + strlen("--exec=");
		else if (leads(argv[argi], "--execute="))
			execute = argv[argi] + strlen("--execute=");
		else if (strcmp(argv[argi], "-m") == 0)
			mm = argv[++argi];
		else if (leads(argv[argi], "--mmap="))
			mm = argv[argi] + strlen("--mmap=");
		else if (strcmp(argv[argi], "-t") == 0)
			transmute = 1;
		else if (strcmp(argv[argi], "--transmute") == 0)
			transmute = 1;
		else if (*argv[argi] == '-') {
			fprintf(stderr, "Invalid argument \"%s\".\n",
				argv[argi]);
			exit(1);
		}
		/*
		 * Indicates the start of filenames.
		 */
		else
			break;
	}
	if (argi >= argc) {
		fprintf(stderr, "No files specified.\n");
		exit(1);
	}

	if (access != NULL && rules(access)) {
		fprintf(stderr, "Access label \"%s\" violates SMACK label rules.\n",
			access);
		exit(1);
	}
	if (mm != NULL && rules(mm)) {
		fprintf(stderr, "mmap label \"%s\" violates SMACK label rules.\n",
			mm);
		exit(1);
	}
	if (execute != NULL && rules(execute)) {
		fprintf(stderr, "execute label \"%s\" violates SMACK label rules.\n",
			execute);
		exit(1);
	}
	for (; argi < argc; argi++) {
		if (access == NULL && mm == NULL &&
		    execute == NULL && !transmute) {
			printf("%s", argv[argi]);
			rc = lgetxattr(argv[argi], "security.SMACK64",
				buffer, LSIZE + 1);
			if (rc > 0) {
				buffer[rc] = '\0';
				printf(" access=\"%s\"", buffer);
			}
			rc = lgetxattr(argv[argi], "security.SMACK64EXEC",
				buffer, LSIZE + 1);
			if (rc > 0) {
				buffer[rc] = '\0';
				printf(" execute=\"%s\"", buffer);
			}
			rc = lgetxattr(argv[argi], "security.SMACK64MMAP",
				buffer, LSIZE + 1);
			if (rc > 0) {
				buffer[rc] = '\0';
				printf(" mmap=\"%s\"", buffer);
			}
			rc = lgetxattr(argv[argi], "security.SMACK64TRANSMUTE",
				buffer, LSIZE + 1);
			if (rc > 0) {
				buffer[rc] = '\0';
				printf(" transmute=\"%s\"", buffer);
			}
			printf("\n");
			continue;
		}
		if (access != NULL) {
			rc = lsetxattr(argv[argi], "security.SMACK64",
				access, strlen(access) + 1, 0);
			if (rc < 0)
				perror(argv[argi]);
		}
		if (execute != NULL) {
			rc = lsetxattr(argv[argi], "security.SMACK64EXEC",
				execute, strlen(execute) + 1, 0);
			if (rc < 0)
				perror(argv[argi]);
		}
		if (mm != NULL) {
			rc = lsetxattr(argv[argi], "security.SMACK64MMAP",
				mm, strlen(mm) + 1, 0);
			if (rc < 0)
				perror(argv[argi]);
		}
		if (transmute) {
			rc = lsetxattr(argv[argi], "security.SMACK64TRANSMUTE",
				"TRUE", 4, 0);
			if (rc < 0)
				perror(argv[argi]);
		}
	}
	exit(0);
}
