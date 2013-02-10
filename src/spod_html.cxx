#include <spod_html.h>
#include <stdlib.h>


#ifdef _MSC_VER
#ifndef WIN32
#define WIN32
#define S_IWUSR 0
#define S_IRUSR 0
#endif
#endif

#ifndef WIN32
#include <unistd.h>
#include <string.h>
#endif

// TODO
// implementation of =begin/=end.
// image_parse --> In spod_parser.cxx
// syn_parse --> In spod_parser.cxx
// Literate programming this
// API documentation this

// $Id: spod_html.cxx,v 1.29 2005/08/02 11:41:10 cvs Exp $

#define DEBUG(a,b)	SPODDBG(a,b)

#ifdef WIN32
#   define DEVNULL	"NUL:"
#   include <io.h>
#   include <fcntl.h>
#   include <sys/stat.h>
static int mkstemp(char *name)
{
  return open(name,O_RDWR|O_CREAT,S_IWUSR|S_IRUSR);
}
#else
#	define DEVNULL "/dev/null"
#endif

spod_html::spod_html(int in,const std::string  & comment, const std::string & prefix, const std::string & postfix)
  : spod_parser(in,comment)
{
  _link_prefix=prefix;
  _link_postfix=postfix;
  _in_pre_sequence=false;
  _in_p_sequence=false;
  _in_item_sequence=false;
  _item_type="";
  _in_table=false;


  {
    std::string cmd("enscript --version >");
    cmd=cmd+DEVNULL+" 2>"+DEVNULL;
    int a=system(cmd.c_str());
    _have_enscript=(a==0);
    if (_have_enscript) {
      char *temp=getenv("TEMP");
      if (temp==NULL) {
	char *templte=strdup("c:/TEMP/spod2htmlXXXXXX");
	int fh=mkstemp(templte);
	if (fh!=-1) {
	  _tempfile=templte;
	  close(fh);
	}
	else {
	  char *templte=strdup("/tmp/spod2htmlXXXXXX");
	  int fh=mkstemp(templte);
	  if (fh!=-1) {
	    _tempfile=templte;
	    close(fh);
	  }
	  else {
	    _have_enscript=false;
	    warning("enscript cannot be started because I cannot make a temporary file");
	  }
	  free(templte);
	}
	free(templte);
      }
      else {
	std::string t(temp);
	t=t+"/spod2htmlXXXXXX";
	char *templte=strdup(t.c_str());
	int fh=mkstemp(templte);
	if (fh!=-1) {
	  _tempfile=templte;
	  close(fh);
	}
	else {
	  _have_enscript=false;
	  warning("enscript cannot be started because I cannot make a temporary file");
	}
	free(templte);
      }
    }
  }
}

spod_html::~spod_html()
{
}

void spod_html::parse_init(void)
{
}

void spod_html::parse_finish(std::string & body)
{
}


bool spod_html::parse_directive(const std::string & directive,const std::string & block,std::string & body)
{

  DEBUG(3,fprintf(stderr,"%s:%s %ld\n",directive.c_str(),block.c_str(),body.size()));
  // Ending pre

  if (directive!="=pre") {
    if (_in_pre_sequence) {
      if (_have_enscript) {
	std::string file=_tempfile+"."+syntax();
	std::string hfile=file+".html";

	FILE *F=fopen(file.c_str(),"wt");
	fprintf(F,"%s",_pre.c_str());
	fclose(F);

	{
	  std::string cmd="enscript --color --quiet --highlight --language=html --output="+hfile+" "+file;
	  if (system(cmd.c_str())!=0) {
	    warning("'%s' went wrong.",cmd.c_str());
	  }


	  FILE *F=fopen(hfile.c_str(),"rt");
	  if (F!=NULL) {
	    std::string n_pre;
	    char buf[8192];
	    bool inpre=false;
	    while(fgets(buf,8191,F)!=NULL) {
	      int idx,idx1;
	      std::string line=buf;
	      idx=line.find("<PRE>");
	      idx1=line.find("</PRE>");
	      if (idx!=-1) {
		inpre=true;
		line=line.substr(idx+5);
		if (ltrim(line)!="") { n_pre+=line; }
	      }
	      else if (idx1!=-1) {
		inpre=false;
		line=line.substr(0,idx1);
		n_pre+=line;
	      }
	      else if (inpre) {
		int i,t,ts;
		std::string ln;

		ts=tabsize();

		for(i=0,t=0;i<(int) line.size();i++) {
		  if (line[i]=='\t') {
		    ln+=" ";
		    t+=1;
		    for(;t%ts!=0;t++) { ln+=" "; }
		  }
		  else {
		    ln+=line[i];
		    t+=1;
		  }
		}

		n_pre+=ln;
	      }
	    }
	    fclose(F);
	    body+=n_pre;
	  }
	  else {
	    body+=escape(_pre);
	  }
	}
      }
      else {
	body+=escape(_pre);
      }

      body+="</PRE>\n";
      _in_pre_sequence=false;
    }
  }

  // general parsing

  if (directive.substr(0,6)=="=table") {

    if (_in_table) {
      body+="</TABLE>\n";
      _in_table=false;
    }
    else {
      std::string caption;
      int         border,frame;

      _in_table=true;
      _table_cell=0;
      _table_row=0;

      if (parse_table(block,_cell_widths,caption,border,frame)) {
	body+="<TABLE";
	if (border>=0) {
	  char s[100];
	  sprintf(s," border=\"%d\"",border);
	  body+=s;
	}
	if (frame>=0) {
	  char s[100];
	  sprintf(s," frame=\"%d\"",frame);
	  body+=s;
	}
	body+=">\n";

	if (caption!="") {
	  body+="<CAPTION>"+caption+"</CAPTION>";
	}
      }
      else {
	body+="<TABLE>";
      }
    }

    DEBUG(7,fprintf(stderr,"=table: _intable=%d, widths.len=%ld\n",_in_table,_cell_widths.size()));

    return true; // continue parsing
  }
  else if (directive.substr(0,5)=="=cell" ||
	   directive.substr(0,6)=="=lcell" ||
	   directive.substr(0,6)=="=rcell" ||
	   directive.substr(0,6)=="=ccell" ||
	   directive.substr(0,6)=="=hcell") {

    DEBUG(7,fprintf(stderr,"=table: _table_cell=%d, _table_row=%d\n",_table_cell,_table_row));

    if (_table_cell==0) {
      if (_table_row>0) {
	if (_cell_type==CT_HEADER) {
	  body+="</TH></TR>";
	}
	else {
	  body+="</TD></TR>";
	}
      }
      body+="<TR>";
    }
    else {
      if (_cell_type==CT_HEADER) {
	body+="</TH>";
      }
      else {
	body+="</TD>";
      }
    }

    if (directive.substr(0,6)=="=hcell") {
      char s[200];
      sprintf(s,"<TH width=\"%d%%\">",_cell_widths[_table_cell]);
      body+=s;
      _cell_type=CT_HEADER;
    }
    else {
      char s[200];
      const char *A="left";
      if (directive.substr(0,6)=="=rcell") { A="right"; }
      else if (directive.substr(0,6)=="=ccell") { A="center"; }
      sprintf(s,"<TD align=\"%s\" width=\"%d%%\">",A,_cell_widths[_table_cell]);
      body+=s;
      _cell_type=CT_NORMAL;
    }

    body+=block;

    _table_cell+=1;
    if (_table_cell>=(int) _cell_widths.size()) {
      _table_cell=0;
      _table_row+=1;
    }

    return true; // continue
  }
  else if (directive.substr(0,4)=="=row") {
    if (_in_table) {
      _table_cell=0;
      _table_row+=1;
    }

    return true;
  }
  else if (directive.substr(0,5)=="=head") {
    int level=to_int(directive.substr(5));

    DEBUG(4,fprintf(stderr,"%s %d %s\n",directive.c_str(),level,block.c_str()));

    if (_head_levels.empty() || level>_head_levels.top()) {
      int lv;
      std::string txt,id;
      current_toc(lv,txt,id);
      DEBUG(4,fprintf(stderr,"!%s - %s - %s\n",directive.c_str(),txt.c_str(),id.c_str()));
      _head_levels.push(level);
      std::string newbody,oldbody;
      oldbody=body;
      parse_body(newbody);
      {char hs[10],he[10];
      DEBUG(4,fprintf(stderr,"!%s - %s - %s\n",directive.c_str(),txt.c_str(),id.c_str()));
      sprintf(hs,"<H%d>",level);
      sprintf(he,"</H%d>\n",level);
      std::string ln=block;
      trim(ln);
      body=oldbody+"<A NAME=\""+id+"\"></A><A HREF=\"#top\">"+hs+ln+he+"</A>"+newbody;
      }
      _head_levels.pop();
      return true;		// continue parsing
    }
    else {
      push_back_block();
      return false; 		// end block
    }
  }
  else if (directive.substr(0,5)=="=over") {
    int amount=to_int(block),level;
    if (_item_levels.empty()) { level=0; }
    else { level=_item_levels.top(); }
    level+=amount;
    DEBUG(4,fprintf(stderr,"%s - %d\n",block.c_str(),level));
    _item_levels.push(level);
    {
      char indent[100];
      std::string oldbody=body,newbody;
      bool old_item_seq=_in_item_sequence;
      std::string old_item_type=_item_type;
      _in_item_sequence=false;
      sprintf(indent,"<div class=\"indent%d\">",level);
      parse_body(newbody);
      body=oldbody+indent+newbody+"</div>";
      _in_item_sequence=old_item_seq;
      _item_type=old_item_type;
    }
    _item_levels.pop();
    return true;		// continue parsing
  }
  else if (directive.substr(0,5)=="=back") {
    if (_in_item_sequence) {
      if (_item_type=="dot") { body+="</UL>\n"; }
      else if (_item_type=="number") { body+="</OL>\n"; }
    }
    return false;		// end block
  }
  else if (directive.substr(0,5)=="=item") {
    if (!_in_item_sequence) {
      _in_item_sequence=true;
      _item_type=get_item_type(block);
      if (_item_type=="dot") { body+="<UL>\n"; }
      else if (_item_type=="number") { body+="<OL>\n"; }
    }

    if (_item_type=="dot" || _item_type=="number") {
      body+="<LI>";
    }
    else if (_item_type=="text") {
      body+="<div class=\"itemindent\"><div class=\"itemcolor\">";
      body+=block;
      body+="</div></div>\n";
    }

    return true;
  }
  else if (directive=="=for") {
    std::string parameter,newblock;
    get_parameters(block,parameter,newblock);
    parameter=tolower(parameter);

    if (parameter=="html") {
      body+=newblock;
    }
    else if (parameter=="comment") {
      body+="\n<!-- "+newblock+" -->\n";
    }
    else {
      warning("format specifier '%s' unknown, skipped",parameter.c_str());
    }
    return true;
  }
  else if (directive=="=begin") {
    // This can be recursive, if parameter starts with a colon.
    // But we only recognize "html" and "comment".
    warning("=begin has not been implemented yet");
    return true;
  }
  else if (directive=="=end") {
    warning("=end has not been implemented yet");
    return true;
  }
  else if (directive=="=image") {
    std::string img_descr,img_name,img_url,img_type,img_align,B;

    if (parse_image(block,img_descr,img_name,img_url,img_type,img_align)) {
      B="<IMG SRC=\""+img_url+"\" alt=\""+img_descr+"\" style=\"float:"+img_align+";margin: 8px;\"/>";
      if (img_align=="center") { B="<CENTER>"+B+"</CENTER>"; }

      img_name=link_id(img_name);
      B="<A NAME=\""+img_name+"\">"+B+"</A>";

      body+=B;
    }

    return true;
  }
  else if (directive=="=syn") {
    std::vector<std::string> syn_parts;
    split(ltrim(block),",",syn_parts);
    if (syn_parts.size()<1) {
      warning("=syn needs parameters <extension>,<tabsize>");
    }
    else {
      set_syntax(ltrim(syn_parts[0]));
      if (syn_parts.size()==1) {
	set_tabsize(8);
      }
      else if (is_uint(syn_parts[1])) {
	set_tabsize(atoi(syn_parts[1].c_str()));
      }
      else {
	warning("=syn parameter <tabsize> must be numeric");
      }
    }

    return true;
  }
  else if (directive=="=pre") {
    if (!_in_pre_sequence) {
      body+="<PRE>\n";
      _in_pre_sequence=true;
      _pre=block;
    }
    else {
      _pre+="\n";
      _pre+=block;
    }

    return true;	// continue parsing
  }
  else if (directive=="=hline") { // horizontal line
    body+="<hr>"+block;
    return true;	// continue parsing
  }
  else if (directive=="=p") {
    body+="<p>"+block+"</p>";
    return true;		// continue parsing
  }
  else if (directive=="=cut") {
    return true;		// continue parsing
  }
  else {
    warning("'%s' is an unknown directive for block '%s'",
	    directive.c_str(),
	    block.c_str()
	    );
	body+="<p>"+directive+" "+block+"</p>";
    return true;		// continue parsing
  }
}


std::string spod_html::parse_escape(const std::string & escape,const std::string & part)
{
  std::string R;

  if (escape=="E") {
    if (part=="lt") { R="&lt;"; }
    else if (part=="gt") { R ="&gt;"; }
    else if (part=="sol") { R="/"; }
    else if (part=="verbar") { R="|"; }
    else if (part=="lchevron") { R="&laquo;"; }
    else if (part=="rchevron") { R="&raquo;"; }
    else if (part=="lb") { R="<br>"; }
    else if (is_uint(part)) { R="&#"+part; }
    else { R="&"+part+";"; }
  }
  else if (escape=="B") {
    R="<B>"+part+"</B>";
  }
  else if (escape=="U") {
    R="<U>"+part+"</U>";
  }
  else if (escape=="C") {
    R="<CODE>"+part+"</CODE>";
  }
  else if (escape=="I" || escape=="F") {
    R="<I>"+part+"</I>";
  }
  else if (escape=="X") {
    R="<A NAME=\""+part+"\"></A>";
  }
  else if (escape=="Z") {
    R="<!-- "+part+" -->";
  }
  else if (escape=="F") {
    R="<CODE>"+part+"</CODE>";
  }
  else if (escape=="S") {
    int i,N;
    for(i=0,N=part.size();i<N;i++) {
      if (part[i]==' ') { R+="&#160;"; }
      else { R+=part[i]; }
    }
  }
  else if (escape=="T") {
    R="<TD>"+part+"</TD>";
  }
  else if (escape=="R") {
    R="<TR>"+part+"</TR>";
  }
  else if (escape=="H") {
    R="<TH>"+part+"</TH>";
  }
  else if (escape=="L") {
    std::vector<std::string> link_parts;
    if (parse_link(part,link_parts)) {
      std::string link_type(link_parts[2]),
	doc_link(link_parts[3]);
      std::string _link_prefix;

      R=generate_link(link_type,link_parts[0],doc_link,link_parts[1]);
    }
    else {
      R="<I>Unknown link</I>";
    }
  }
  else if (escape==" ") {
    R=" &lt;"+part+"&gt;";
  }
  else {
    warning("'%s' is an unknown escape for part '%s'",escape.c_str(),part.c_str());
    R=escape+"&lt;"+part+"&gt;";		// continue parsing
  }

  return R;
}

std::string spod_html::escape(const std::string & block)
{
  int i,N,p;
  std::string NB,esc;
  for(p=i=0,N=block.size();i<N;i++) {
    switch (block[i]) {
    case '<': esc="&lt;";
      break;
    case '>': esc="&gt;";
      break;
    case '&': esc="&amp;";
      break;
    }

    if (esc!="") {
      NB+=block.substr(p,i-p)+esc;
      esc="";
      p=i+1;
    }
  }
  if (p<i) {
    NB+=block.substr(p);
  }
  return NB;
}


std::string spod_html::generate_link(std::string & link_type,std::string & link, std::string & internal_link,std::string & description)
{
  std::string R;

  if (internal_link!="") { internal_link="#"+internal_link; }

  if (link_type=="internal") {
    R="<A HREF=\""+internal_link+"\">"+description+"</A>";
  }
  else if (link_type=="external") {
    R="<A HREF=\""+_link_prefix+link+_link_postfix+internal_link+"\">"+description+"</A>";
  }
  else if (link_type=="fullurl") {
    R="<A HREF=\""+link+internal_link+"\">"+description+"</A>";
  }
  else {
    warning("Unknown link type '%s'",link_type.c_str());
    R="<I>Unkown link type '";
    R+=link_type;
    R+="'</I>";
  }

  return R;
}

