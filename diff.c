//
//  diff_02.c
//  diff
//
//  Created by William McCarthy on 4/29/19.
//  Copyright © 2019 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>

#include "para.h"
//  para.c
//  diff
//
//  Created by William McCarthy on 5/9/19.
//  Copyright © 2019 William McCarthy. All rights reserved.

#define BUFLEN 256

char buf[BUFLEN];
char *strings1[MAXSTRINGS], *strings2[MAXSTRINGS];
int showversion = 0, showbrief = 0, ignorecase = 0, report_identical = 0, showsidebyside = 0;
int showleftcolumn = 0, showunified = 0, showcontext = 0, suppresscommon = 0, diffnormal = 0;

int count1 = 0, count2 = 0;

//
//  util.c
//  diff
//
//  Created by William McCarthy on 5/9/19.
//  Copyright © 2019 William McCarthy. All rights reserved.
//



char* yesorno(int condition) { return condition == 0 ? "no" : "YES"; }

FILE* openfile(const char* filename, const char* openflags) {
  FILE* f;
  if ((f = fopen(filename, openflags)) == NULL) {  printf("can't open '%s'\n", filename);  exit(1); }
  return f;
}

void printline(void) {
  for (int i = 0; i < 10; ++i) { printf("=========="); }
  printf("\n");
}


void printleft(const char* left) {
  char buf[BUFLEN];

  if (showsidebyside) {
    strcpy(buf, left);
    int j = 0, len = (int)strlen(buf) - 1;
    for (j = 0; j <= 48 - len ; ++j) { buf[len + j] = ' '; }
    buf[len + j++] = '<';
    buf[len + j++] = '\0';
    printf("%s\n", buf);
  }
  else if (showcontext) {
    printf("%s %s", "-", left);
  }
  else printf("%s %s", "<", left);
}

void printright(const char* right) {
  if (right == NULL) { return; }
  if (showsidebyside) {
    printf("%50s %s", ">", right);}
  else if (showcontext) {
    printf("%s %s", "+", right);
  }
  else printf("%s %s", ">", right);
}

void printboth(const char* left, const char* right) {
  if (showsidebyside) {
    char buf[BUFLEN];
    size_t len = strlen(left);
    if (len > 0) { strncpy(buf, left, len); }
    buf[len - 1] = '\0';
    if (suppresscommon) {
      if (strcmp(left, right) != 0) {printf("%-48s %s %s", buf, "|", right); }
    }
    else if (showleftcolumn) {
      if (strcmp(left, right) == 0) { printf("%-48s %s\n", buf , "("); }
      else printf("%-48s %s %s", buf, "|", right);
    }
    else {
      if (strcmp(left, right) == 0) { printf("%-48s %s %s",buf , " " ,right); }
      else printf("%-48s %s %s", buf, "|", right);
    }
  }
  else if (showcontext) {
    if (strcmp(left, right) == 0)  { printf("%s %s", " ", left);}
       else printf("%s %s" , "!", left);
  }

  else {
    if (strcmp(left, right) == 0) { }
    else printf("< %s---\n> %s", left, right);
  }
}

para* para_make(char* base[], int filesize, int start, int stop) {
  para* p = (para*) malloc(sizeof(para));
  p->base = base;
  p->filesize = filesize;
  p->start = start;
  p->stop = stop;
  p->firstline = (p == NULL || start < 0) ? NULL : p->base[start];
  p->secondline = (p == NULL || start < 0 || filesize < 2) ? NULL : p->base[start + 1];

  return p;
}

para* para_first(char* base[], int size) {
  para* p = para_make(base, size, 0, -1);
  return para_next(p);
}

void para_destroy(para* p) { free(p); }

para* para_next(para* p) {
  if (p == NULL || p->stop == p->filesize) { return NULL; }

  int i;
  para* pnew = para_make(p->base, p->filesize, p->stop + 1, p->stop + 1);
  for (i = pnew->start; i < p->filesize && strcmp(p->base[i], "\n") != 0; ++i) { }
  pnew->stop = i;

  if (pnew->start >= p->filesize) {
    free(pnew);
    pnew = NULL;
  }
  return pnew;
}
size_t para_filesize(para* p) { return p == NULL ? 0 : p->filesize; }

size_t para_size(para* p) { return p == NULL || p->stop < p->start ? 0 : p->stop - p->start + 1; }

char** para_base(para* p) { return p->base; }

char* para_info(para* p) {
  static char buf[BUFLEN];   // static for a reason
  snprintf(buf, sizeof(buf), "base: %p, filesize: %d, start: %d, stop: %d\n",
           p->base, p->filesize, p->start, p->stop);
  return buf;  // buf MUST be static
}

int para_equal(para* p, para* q) {
  if (p == NULL || q == NULL) { return 0; }
  if (para_size(p) != para_size(q)) { return 0; }
  if (p->start >= p->filesize || q->start >= q->filesize) { return 0; }
  int i = p->start, j = q->start, equal = 0;
  while ((equal = strcmp(p->base[i], q->base[i])) == 0) { ++i; ++j; }
  return 1;
}

void para_print(para* p, void (*fp)(const char*)) {
  if (p == NULL) { return; }
  for (int i = p->start; i <= p->stop && i != p->filesize; ++i) { fp(p->base[i]);  }
}

void para_printboth(para* p, para* q, void (*fp)(const char*, const char*)) {
  int i, j;
  // start1 = 0
  if (p == NULL || q == NULL) { return; }
  for (i = p->start, j = q->start; (i <= p->stop && i != p->filesize) && (j <= q->stop && j != q->filesize); ++i, ++j)
   { fp(p->base[i], q->base[j]);}
}

void para_printfile(char* base[], int count, void (*fp)(const char*)) {
  para* p = para_first(base, count);
  while (p != NULL) {
    para_print(p, fp);
    p = para_next(p);
  }
  printline();
}


void version(void) {
  printf("\ndiff (CSUF diffutils) 1.0.0\n");
  printf("Copyright (C) 2014 CSUF\n");
  printf("This program comes with NO WARRANTY, to the extent permitted by law.\n");
  printf("You may redistribute copies of this program\n");
  printf("under the terms of the GNU General Public License.\n");
  printf("For more information about these matters, see the file named COPYING.\n");
  printf("Written by William McCarthy, Zhi Zeng.\n");
}

void todo_list(void) {
  printf("\n\n\nTODO: check line by line in a paragraph, using '|' for differences");
  printf("\nTODO: this starter code does not yet handle printing all of fin1's paragraphs.");
  printf("\nTODO: handle the rest of diff's options\n");
}


void loadfiles(const char* filename1, const char* filename2) {
  memset(buf, 0, sizeof(buf));
  memset(strings1, 0, sizeof(strings1));
  memset(strings2, 0, sizeof(strings2));

  FILE *fin1 = openfile(filename1, "r");
  FILE *fin2 = openfile(filename2, "r");

  while (!feof(fin1) && fgets(buf, BUFLEN, fin1) != NULL) { strings1[count1++] = strdup(buf); }  fclose(fin1);
  while (!feof(fin2) && fgets(buf, BUFLEN, fin2) != NULL) { strings2[count2++] = strdup(buf); }  fclose(fin2);
}

void print_option(const char* name, int value) { printf("%17s: %s\n", name, yesorno(value)); }

void diff_output_conflict_error(void) {
  fprintf(stderr, "diff: conflicting output style options\n");
  fprintf(stderr, "diff: Try `diff --help' for more information.)\n");
  exit(CONFLICTING_OUTPUT_OPTIONS);
}

void setoption(const char* arg, const char* s, const char* t, int* value) {
  if ((strcmp(arg, s) == 0) || ((t != NULL && strcmp(arg, t) == 0))) {
    *value = 1;
  }
}

void showoptions(const char* file1, const char* file2) {
  printf("diff options...\n");
  print_option("diffnormal", diffnormal);
  print_option("show_version", showversion);
  print_option("show_brief", showbrief);
  print_option("ignorecase", ignorecase);
  print_option("report_identical", report_identical);
  print_option("show_sidebyside", showsidebyside);
  print_option("show_leftcolumn", showleftcolumn);
  print_option("suppresscommon", suppresscommon);
  print_option("showcontext", showcontext);
  print_option("show_unified", showunified);

  printf("file1: %s,  file2: %s\n\n\n", file1, file2);

  printline();
}

const char* files[2] = { NULL, NULL };
void init_options_files(int argc, const char* argv[]) {
  int cnt = 0;

  while (argc-- > 0) {
    const char* arg = *argv;
    setoption(arg, "-v",       "--version",                  &showversion);
    setoption(arg, "-q",       "--brief",                    &showbrief);
    setoption(arg, "-i",       "--ignore-case",              &ignorecase);
    setoption(arg, "-s",       "--report-identical-files",   &report_identical);
    setoption(arg, "--normal", NULL,                         &diffnormal);
    setoption(arg, "-y",       "--side-by-side",             &showsidebyside);
    setoption(arg, "--left-column", NULL,                    &showleftcolumn);
    setoption(arg, "--suppress-common-lines", NULL,          &suppresscommon);
    setoption(arg, "-c",       "--context",                  &showcontext);
    setoption(arg, "-u",       "showunified",                &showunified);
    if (arg[0] != '-') {
      if (cnt == 2) {
        fprintf(stderr, "apologies, this version of diff only handles two files\n");
        fprintf(stderr, "Usage: ./diff [options] file1 file2\n");
        exit(TOOMANYFILES_ERROR);
      } else { files[cnt++] = arg; }
    }
    ++argv;   // DEBUG only;  move increment up to top of switch at release
  }

  if (!showcontext && !showunified && !showsidebyside && !showleftcolumn) {
    diffnormal = 1;
  }

  if (showversion) { version();  exit(0); }

  if (((showsidebyside || showleftcolumn) && (diffnormal || showcontext || showunified)) ||
      (showcontext && showunified) || (diffnormal && (showcontext || showunified))) {

    diff_output_conflict_error();
  }

//  showoptions(files[0], files[1]);
  loadfiles(files[0], files[1]);
}

void reportiden(para *p, para* q) {
  if (*files[0] == *files[1]) { printf("Files %s and %s are identical\n",files[0], files[1]); }
  else { search(p, q); }

}

void filetime(void) {
  struct stat t;
  struct stat s;
  stat(files[1],&s);
  stat(files[0],&t);
  struct tm* ti = localtime(&t.st_mtime);
  struct tm* si = localtime(&s.st_mtime);
  if(showcontext) {
    printf("*** %-10s %4d-%02d-%02d %02d:%02d:%012.9f\n", files[0], ti->tm_year+1900, ti->tm_mon+1, ti->tm_mday,
          ti->tm_hour, ti->tm_min, (float)ti->tm_sec  );
    printf("--- %-10s %4d-%02d-%02d %02d:%02d:%012.9f\n", files[1], si->tm_year+1900, si->tm_mon+1, si->tm_mday,
                si->tm_hour, si->tm_min, (float)si->tm_sec  );
  }
  if(showunified) {
    printf("--- %-10s %4d-%02d-%02d %02d:%02d:%012.9f\n", files[0], ti->tm_year+1900, ti->tm_mon+1, ti->tm_mday,
          ti->tm_hour, ti->tm_min, (float)ti->tm_sec  );
    printf("+++ %-10s %4d-%02d-%02d %02d:%02d:%012.9f\n", files[1], si->tm_year+1900, si->tm_mon+1, si->tm_mday,
                si->tm_hour, si->tm_min, (float)si->tm_sec  );}
}
void briefmode(para* p, para* q) {
  if (!para_same()) { printf("Files %s and %s differ\n",files[0], files[1] );}
  else if (report_identical)  {reportiden(p, q); }
}

int para_same(void) {
  if (*files[0] == *files[1]) {
    exit(0); }
  else return 0;
}



void search(para* p, para* q) {
  if (*files[0] == *files[1] && showsidebyside) {
    while (p != NULL && q != NULL) {
    para_printboth(p, q, printboth);
    p = para_next(p);
    q = para_next(q);
  }
  exit(0);
}
  para_same();
  filetime();
  int foundmatch = 0;
  para* qlast = q;
  while (p != NULL) {
    qlast = q;
    foundmatch = 0;
    while (q != NULL && (foundmatch = para_equal(p, q)) == 0) {
      q = para_next(q);
    }
    q = qlast;

    if (foundmatch) {
      while ((foundmatch = para_equal(p, q)) == 0) {
        para_print(q, printright);
        q = para_next(q);
        qlast = q;
      }
      para_printboth(p, q, printboth);
      p = para_next(p);
      q = para_next(q);
    } else {
      para_print(p, printleft);
      p = para_next(p);

    }
  }
  while (q != NULL) {
    para_print(q, printright);
    q = para_next(q);
  }
}


int main(int argc, const char * argv[]) {
  init_options_files(--argc, ++argv);

//  para_printfile(strings1, count1, printleft);
//  para_printfile(strings2, count2, printright);

  para* p = para_first(strings1, count1);
  para* q = para_first(strings2, count2);
  if (showbrief) { briefmode(p, q); }
  else if (report_identical) { reportiden(p, q); }
  else { search(p, q); }

  return 0;
}
