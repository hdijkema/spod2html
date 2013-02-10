/*
 * Copyright (C) 2013 Hans Oesterholt <debian@oesterholt.net>
 * 
 * spod2html is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * spod2html is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <hoptions.h>
#include <spod_html.h>

#include <config.h>

#define DEBUG(a) 	fprintf(stderr,"%s\n",a);fseek(stderr,0,SEEK_END);

void usage(void) {
  fprintf(stderr,
	  "spod2html [<options>] <file in> [<file out>]\n\n"
	  "Version: %s\n\n"
	  "Possible options:\n\n"
	  "  --help                  - Gives this help message\n"
	  "  --version               - Gives the version\n\n"
	  "  --stylesheet=<sheet>    - Link given sheet for CSS\n"
	  "  --comment=<c1|c2|c3...> - Use given comments (separated by pipes)\n"
	  "  --recognize-comments    - Recognize comments by file extension\n\n"
	  "  --toc=<toc header>      - Use given Table Of Contents header (default: Contents)\n"
	  "  --notoc                 - Do not generate table of contents\n\n"
	  "  --body-only             - Only output the HTML body\n"
	  "  --link-prefix           - Use given prefix for links\n"
	  "  --link-postfix          - Use given postfix for linke (e.g. \".html\")\n"
	  "  --title-out=<file>      - Output the document title to file\n\n"
	  ,   VERSION
	  );
}

void version(void)
{
  printf("v%s\n",VERSION);
}

void err(const std::string & msg)
{
  fprintf(stderr,"ERROR: %s\n",msg.c_str());
  exit(2);
}

std::string in(const std::string & ext,const char *type,...)
{
  va_list     ap;
  std::string R;
  va_start(ap,type);
  while(type!=NULL && R=="") {
    std::string exts=type;
    int         i;
    while((i=exts.find("|"))!=-1 && R=="") {
      std::string e=exts.substr(0,i);
      exts=exts.substr(i+1);
      if (e==ext) {
	type=va_arg(ap,char *);
	R=type;
	type=va_arg(ap,char *);
      }
    }
    if (R=="" && exts!="") {
      if (ext==exts) {
	type=va_arg(ap,char *);
	R=type;
	type=va_arg(ap,char *);
      }
    }

    if (R=="") {
      type=va_arg(ap,char *);
      type=va_arg(ap,char *);
    }
  }
  va_end(ap);
  return R;
}

std::string get_comments(const std::string & filen)
{
  std::string ext,R;
  int         i;

  i=filen.rfind(".");
  if (i==-1) { err("Cannot determine filetype of "+filen); }
  ext=filen.substr(i+1);
  if (ext=="") { err("Cannot determine filetype of "+filen); }

  R=in(ext,"scm",";",
       "c|cxx|cpp|c++|cc|h|hxx|hpp|h++","//|/*",
       "pl|pm","#",
       "php|php3|php4|php5","//|/*|#",
       "sql","--",
       "pod","NIL",
       NULL,NULL
       );

  if (R=="") {
    err("I know nothing of the filetype of "+filen);
  }

  if (R=="NIL") { R=""; }

  return R;
}


#define OPTIONS \
   "help|version|" \
   "stylesheet|" \
   "comment|recognize-comments|" \
   "toc|notoc|" \
   "body-only|link-prefix|link-postfix|title-out||debug"

int main(int argc,char *argv[])
{
  hoptions  	o(argc,argv,OPTIONS);
  bool      	notoc,body_only;
  std::string   stylesheet,comments,toc_name,link_prefix,link_postfix,title_file;
  std::string   file_in,file_out;
  int           debug_level;

  // Getting options.

  //	o.info();

  if (!o.ok()) 			{ usage();err(o.error()); }
  if (o.get("help")) 		{ usage();exit(0); }
  if (o.get("version")) 	{ version();exit(0); }
  if (o.size()<1 || o.size()>2) { usage();exit(1); }

  file_in=o[0];
  if (o.size()==2) { file_out=o[1]; }
  else { file_out=file_in+".html"; }

  notoc		= o.get("notoc");
  body_only	= o.get("body-only");
  o.get("stylesheet",stylesheet);
  o.get("comment",comments);
  if (o.get("recognize-comments")) { comments=get_comments(file_in); }
  o.get("toc",toc_name);if (toc_name=="") { toc_name="Contents"; }
  o.get("link-prefix",link_prefix);
  o.get("link-postfix",link_postfix);
  o.get("title-out",title_file);
  o.get("debug",debug_level);

  // Parsing HTML

  {
    FILE *in=fopen(file_in.c_str(),"rt");
    FILE *out=fopen(file_out.c_str(),"wt");
    if (in==NULL) { err("Cannot open file '"+file_in+"'"); }
    if (out==NULL) { err("Cannot open output file '"+file_out+"'"); }

    spod_html H(fileno(in),comments,link_prefix,link_postfix);

    H.debug(debug_level);
    H.parse();

    if (!body_only) {
      fprintf(out,"<html>\n");
      fprintf(out,"<title>%s</title>\n",H.title().c_str());
      if (stylesheet!="") {
	fprintf(out,"<link rel=\"stylesheet\" href=\"%s\" type=\"text/css\">\n",stylesheet.c_str());
      }
      fprintf(out,"<body>\n");
    }

    fprintf(out,"<div class=\"spod\">");

    if (!notoc) {
      fprintf(out,"<div class=\"toc\">");
      fprintf(out,"<A NAME=\"top\"></A><H1>%s</H1>\n",toc_name.c_str());
      {int i;
      int level;
      std::string text,id;
      for(i=0;H.toc(i,level,text,id);i++) {
	fprintf(out,"<DIV class=\"indent%d\">",level);
	fprintf(out,"<A HREF=\"#%s\">%s</A>",
		id.c_str(),text.c_str()
		);
	fprintf(out,"</DIV>\n");
      }
      }
      fprintf(out,"</div>\n");
    }

    fprintf(out,"<div class=\"text\">\n");
    fprintf(out,"%s\n",H.body().c_str());
    fprintf(out,"</div>\n");
    fprintf(out,"</div>\n");

    if (!body_only) {
      fprintf(out,"</body>\n");
      fprintf(out,"</html>\n");
    }

    fclose(in);
    fclose(out);

    if (title_file!="") {
      FILE *out=fopen(title_file.c_str(),"wt");
      if (out) {
	fprintf(out,"%s",H.title().c_str());
	fclose(out);
      }
    }
  }

  return 0;
}
