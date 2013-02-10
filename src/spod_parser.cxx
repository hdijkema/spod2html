#include <spod_parser.h>
#include <ctype.h>

#ifdef _MSC_VER
#ifndef WIN32
#define WIN32
#endif
#endif

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include <stdarg.h>
#include <harray.h>

#define MAX_LINE	20480
#define BUF_SIZE	10240

#define DEBUG(a,b)	SPODDBG(a,b)

spod_parser::spod_parser(int fh,const std::string & _comment)
{
  std::string comment=_comment;

  {
    char buf[BUF_SIZE+1];
    int  bytes;
    while ((bytes=read(fh,buf,BUF_SIZE))!=0) {
      buf[bytes]='\0';
      _in+=buf;
    }

    // To prevent this thing from sigsegv'ing, add =cut\n\n (the last \n is for read_block()).

    while(_in.substr(_in.size()-2)!="\n\n") {
      _in+="\n";
    }
    _in+="=cut\n\n";

    // Convert from DOS to UNIX if necessary

    {int i,j,N;
    char *S;
    N=(int) _in.size();
    S=new char[N+1];
    for(i=0,j=0;i<N;i++) {
      if (_in[i]!='\r') { S[j]=_in[i];j++; }
    }
    S[j]='\0';
    _in=S;
    delete S;
    }
  }

  _length=_in.size();

  {
    trim(comment);
    while (comment!="") {
      int i=comment.find("|");
      if (i==-1) {
	trim(comment);
	if (comment!="") {
	  _comments.insert(_comments.end(),comment);
	}
	comment="";
      }
      else {
	std::string cmnt=ltrim(comment.substr(0,i));
	comment=comment.substr(i+1);
	if (cmnt!="") {
	  _comments.insert(_comments.end(),cmnt);
	}
      }
    }
  }

  _reread=false;
  _line_number=0;
  _syntax="txt";
  _tabsize=8;
  debug();
}

spod_parser::~spod_parser()
{
}

void spod_parser::parse(void)
{
  // Initialize

  DEBUG(1,fprintf(stderr,"init\n"));
  rewind();
  _body="";
  _ispod=false;
  _cursor=0;

  DEBUG(1,fprintf(stderr,"initialization: parse_init\n"));
  parse_init();

  DEBUG(1,fprintf(stderr,"init ready\n"));

  // Stage WikiWiki

  DEBUG(1,fprintf(stderr,"stage 0: convert wikiwikiwiki mode to spod\n"));

  rewind();
  _body="";
  parse_wikiwikiwiki(_body);

  // Stage 1

  DEBUG(1,fprintf(stderr,"stage 1: parse_escapes\n"));

  _in=_body;

  rewind();
  _body="";
  _next_block_is_title=false;

  parse_escapes(_body);

  DEBUG(1,fprintf(stderr,"stage 1 _body size: %ld\n",_body.size()));
  DEBUG(2,fprintf(stderr,"body contents:\n********************\n%s\n********************\n",_body.c_str()));


  // Stage 2
  DEBUG(1,fprintf(stderr,"stage 2 preparation\n"));

  _in=_body;

  rewind();
  _body="";
  _ispod=false;
  _next_block_is_title=false;

  DEBUG(1,fprintf(stderr,"stage 2: parse_body\n"));

  parse_body(_body);

  DEBUG(1,fprintf(stderr,"stage 2 _body size: %ld\n",_body.size()));

  // Enclose pod?
  DEBUG(1,fprintf(stderr,"finishing: parse_finish\n"));

  parse_finish(_body);
}

std::string spod_parser::get_item_type(const std::string & line)
{
  std::string ln;
  std::string R;
  ln=ltrim(line);
  if (ln=="" || ln=="*" || ln=="-") { R="dot"; }
  else {
    int i;
    for(i=0;i<(int) ln.size() && ln[i]>='0' && ln[i]<='9';i++);
    if (i==(int) ln.size()) { R="number"; }
    else if ((ln[i]=='.' || ln[i]==')') && i==(int) ln.size()-1) { R="number"; }
    else { R="text"; }
  }
  return R;
}



bool spod_parser::parse_image(std::string block,
			      std::string & img_descr,
			      std::string & img_name,
			      std::string & img_url,
			      std::string & img_type,
			      std::string & img_align)
{
    std::vector<std::string> image_parts;
    split(ltrim(block),",",image_parts);

    {
      int i;
      for(i=0;i<(int) image_parts.size();i++) {
	trim(image_parts[i]);
      }
    }

    if (image_parts.size()<5) {
      warning("=image : syntax error, use: =image <description>,<name>,<url>,<type>,<alignment>; provided: =image %s",block.c_str());
      return false;
    }
    else {
      img_descr=image_parts[0];
      img_name=image_parts[1];
      img_url=image_parts[2];
      img_type=image_parts[3];
      img_align=image_parts[4];
      img_align=tolower(img_align);
      return true;
    }
}


bool spod_parser::parse_table(std::string block,
			      std::vector<int> & widths,
			      std::string & caption,
			      int & border,
			      int & frame)
{
  int j,i,N,k;
  std::string _colspec=block;
  harray<std::string> _colspecs;
  std::string _border;
  std::string _frame;
  std::string _caption;

  trim(_colspec);
  DEBUG(7,fprintf(stderr,"=table: starting, colspec=%s\n",_colspec.c_str()));

  for(i=0,N=_colspec.length();i<N && _colspec[i]!=',';i++);
  if (i<N) {
    _caption=_colspec.substr(i+1);
    _colspec=_colspec.substr(0,i);
  }
  for(i=0,N=_caption.length();i<N && _caption[i]!=',';i++);
  if (i<N) {
    _border=_caption.substr(i+1);
    _caption=_caption.substr(0,i);
  }
  for(i=0,N=_border.length();i<N && _border[i]!=',';i++);
  if (i<N) {
    _frame=_border.substr(i+1);
    _border=_border.substr(0,i);
  }

  trim(_colspec);
  if (_colspec!="") {
    _colspec+=" ";
    for(j=0,k=0,i=0,N=_colspec.length();i<N;i++) {
      if (isspace(_colspec[i])) {
	_colspecs[j++]=_colspec.substr(k,i-k);
	for(;i<N && isspace(_colspec[i]);i++);
	k=i;
      }
    }
  }

  trim(_frame);
  trim(_border);
  trim(_caption);

  // output

  widths.clear();
  for(i=0,N=_colspecs.length();i<N;i++) {
    widths.insert(widths.end(),atoi(_colspecs[i].c_str()));
  }
  if (_frame!="") {
    frame=atoi(_frame.c_str());
  }
  else {
    frame=-1;
  }
  if (_border!="") {
    border=atoi(_border.c_str());
  }
  else {
    border=-1;
  }
  caption=_caption;

  if (N==0) {
    warning("=table: syntax error, syntax: =table <widths(%%) of columns>[,caption[,border[,frame]]], provided: =table %s",
	    block.c_str()
	    );
    widths.insert(widths.end(),100);
    return false;
  }
  else {
    return true;
  }

}


bool spod_parser::parse_link(const std::string & _part,std::vector<std::string> & link_parts)
  // returns in vector: link, link_description, link_type, document_internal_link.
{
  std::string link,description,link_type,document_link;
  std::string part(_part);
  int	 i;
  bool link_is_desc;

  trim(part);
  i=part.find("|");
  if (i!=-1) {
    description=part.substr(0,i);
    link=part.substr(i+1);
    link_is_desc=false;
  }
  else {
    link=part;
    link_is_desc=true;
  }

  link_type="external";

  // Link type adjustment

  trim(link);
  {
    int i;
    for(i=0;i<(int) link.size() && ::tolower(link[i])>='a' && ::tolower(link[i])<='z' && link[i]!=':';i++);
    if (i!=(int) link.size() && link[i]==':' && i>0) {
      link_type="fullurl";
    }
  }

  if (link_type!="fullurl") {
    i=link.find("/");
    if (i!=-1) {
      int j,N;
      document_link=link.substr(i+1);
      link=link.substr(0,i);
      for(j=0,N=document_link.size();j<N && document_link[j]=='"';j++);
      document_link=document_link.substr(j);
      for(N=document_link.size(),j=N-1;j>=0 && document_link[j]=='"';j--);
      document_link=document_link.substr(0,j+1);
      if (i==0) {
	link_type=="internal";
      }
    }
  }

  if (link_is_desc) {
    description=link+"/"+document_link;
    if (description[0]=='/') { description=description.substr(1); }
  }

  // Trimming

  trim(link);
  trim(description);
  trim(link_type);
  trim(document_link);


  // Post parsing and returning

  if (document_link!="") { document_link=link_id(document_link); }
  link_parts.insert(link_parts.end(),link);
  link_parts.insert(link_parts.end(),description);
  link_parts.insert(link_parts.end(),link_type);
  link_parts.insert(link_parts.end(),document_link);

  return true;
}

std::string spod_parser::parse_escapes_for_part(const std::string & part,int level)
{
  int from,to,N;
  int escapes;

  DEBUG(4,fprintf(stderr,"recursion level:%d\n",++level));
  DEBUG(4,fprintf(stderr,"part.size=%ld\n",part.size()));
  DEBUG(5,fprintf(stderr,"part=%s\n",part.c_str()));

  for(from=0,N=part.size();from<N && part[from]!='<';from++);
  if (from==N) {
    DEBUG(4,fprintf(stderr,"returns part itself\n"));
    return std::string(part);
  }
  else {
    // find closer
    for(to=from+1,escapes=1;to<N && escapes>0;) {
      if (part[to]=='<') { escapes+=1; }
      if (part[to]=='>') { escapes-=1; }
      if (escapes!=0) { to+=1; }
    }
    if (escapes>0) {
      warning("Unclosed escape sequence");
      escapes=0;
    }

    std::string head,middle,tail,escape;

    if (from>0) { escape=part.substr(from-1,1); }		// keep in mind: C<<< ... >>>can also happen!
    if (from>0) { head=part.substr(0,from-1); }
    DEBUG(4,fprintf(stderr,"parse_escapes_for_part(middle)\n"));
    middle=parse_escapes_for_part(part.substr(from+1,to-from-1),level);
    DEBUG(5,fprintf(stderr,"middle=%s\n",middle.c_str()));
    DEBUG(4,fprintf(stderr,"parse_escape(%s,%s)\n",escape.c_str(),middle.c_str()	));
    if (escape!="") {
      middle=parse_escape(escape,middle);
    }
    DEBUG(5,fprintf(stderr,"middle=%s\n",middle.c_str()));
    if (to<N-1) {
      DEBUG(4,fprintf(stderr,"parse_escapes_for_part(tail)\n"));
      tail=part.substr(to+1);
      DEBUG(5,fprintf(stderr,"tail.size=%ld, tail=%s\n",tail.size(),tail.c_str()));
      tail=parse_escapes_for_part(tail,level);
    }

    DEBUG(5,fprintf(stderr,"%s - %s - %s\n",head.c_str(),middle.c_str(),tail.c_str()));
    return head+middle+tail;
  }
}

void spod_parser::parse_escapes(std::string & body) {
  std::string directive,block;
  bool		tmp;
  std::string pdir,part,original_block;
  bool		ispod=false;

  while(read_block(directive,block,tmp,original_block)) {

    if (directive!="=p" && directive!="=pre" && directive!="=cut") {
      ispod=true;
    }
    else if (directive=="=cut") { ispod=false; }

    DEBUG(6,fprintf(stderr,"%d - %s\n",ispod,block.c_str()));

    if (directive=="=verbatim") {
      body+=original_block;
    }
    else if (ispod &&
	     directive!="=pre" && directive!="=for" && directive!="=begin" && directive!="=end"
	     ) {
      body+=parse_escapes_for_part(original_block,0);
    }
    else {
      body+=original_block;
    }
  }
}

static
int eof_line(std::string & block,int pos)
{
  if (block[pos]=='\r') {
    if (block[pos+1]=='\n') { return 2; }
    else { return 0; }
  }
  else if (block[pos]=='\n') {
    return 1;
  }
  else {
    return 0;
  }
}

static
bool is_url(std::string word)
{
  int i,N=word.length();
  for(i=0;i<N &&
	((word[i]>='a' && word[i]<='z') ||
	(word[i]>='A' && word[i]<='Z'));i++);

  if (i==N) {
    return false;
  }
  else {
    return word[i]==':' && i!=N-1 && !isspace(word[i+1]);
  }
}

static
std::string wikiwiki_escape(std::string word)
{
  int i,N=word.length();
  std::string nword;
  for(i=0;i<N;i++) {
    if (word[i]=='<') {
      nword+="E<lt>";
    }
    else if (word[i]=='>') {
      nword+="E<gt>";
    }
    else {
      nword+=word[i];
    }
  }
  return nword;
}

static
int is_wikiwikiword(std::string word,std::string & w,std::string & r)
{
  int i,N=word.length();
  bool wikiword=false;
  int  caseChanges=0;
  int  theCase,oldCase;

  theCase=(word[0]>='a' && word[0]<='z') ? 0 : (word[0]>='A' && word[0]<='Z') ? 1 : 0;
  oldCase=theCase;

  for(i=1;i<N && !wikiword;i++) {
    theCase=(word[i]>='a' && word[i]<='z') ? 0 : (word[i]>='A' && word[i]<='Z') ? 1 : theCase;
    if (oldCase!=theCase) { caseChanges+=1; }
    oldCase=theCase;
    if (caseChanges>2) { wikiword=true; }
  }

  if (wikiword) {
    int i;

    for(i=0;i<N && (isalnum(word[i]) || word[i]=='-' || word[i]=='_');i++);

    w=word.substr(0,i);
    r=word.substr(i);
  }

  return wikiword;
}

bool spod_parser::is_image_word(std::string & word,std::string & type)
{
  trim(word);

  {
    int N=word.length();

    if (N<5) { return false; }
    else {
      std::string ext=word.substr(N-4);

      type=ext.substr(1);

      DEBUG(7,fprintf(stderr,"is_image_word: %s, %s, %s\n",word.c_str(),ext.c_str(),type.c_str()));

      if (ext==".png") { return true; }
      else if (ext==".jpg") { return true; }
      else if (ext==".gif") { return true; }
      else { return false; }
    }
  }
}

static
bool is_space(const char c)
{
  return isspace(c) || c=='.' || c==',' || c==':' || c==';';
}

static
bool is_sep(const char c)
{
  return c=='*' || c=='/' || c=='*' || c=='[' || c==']' || c=='#' || c=='_';
}

static
bool is_same(const char a,const char b)
{
  return a==b;
}


int spod_parser::is_wikiwiki_start_markup(int i,std::string line,const char *sep)
{
  if (i>0) {
    int k;

    for(k=i-1;k>0 && is_sep(line[k]) && !is_space(line[k]) && !is_same(line[k],sep[0]);k--);
    if (is_space(line[k]) || k==0) {
      if (line[i]==sep[0]) {
	int k;
	int N=line.length();
	for(k=i+1;k<N && is_sep(line[k]) && !is_same(line[i],line[k]);k++);
	if (k==N || is_same(line[i],line[k])) {
	  return 0;
	}
	else {
	  return 1;
	}
      }
      else {
	return 0;
      }
    }
    else {
      return 0;
    }
  }
  else {
    if (line[i]==sep[0]) {
	int k;
	int N=line.length();
	for(k=i+1;k<N && is_sep(line[k]) && !is_same(line[i],line[k]);k++);
	if (k==N || is_same(line[i],line[k])) {
	  return 0;
	}
	else {
	  return 1;
	}
    }
    else {
      return 0;
    }
  }
}

static
std::string remove_wikiwiki_escaped_markup(std::string line)
{
  int i;
  int N=line.length();

  for(i=0;i<N-1;i++) {
    if (line[i]=='!' && (is_sep(line[i+1]) || isalpha(line[i+1]) || isdigit(line[i+1]))) {
      line[i]=' ';
    }
  }

  return line;
}

int spod_parser::is_wikiwiki_end_markup(int i,std::string line,const char *sep)
{
  int N=line.length();

  if (i<N) {
    if (line[i]==sep[0]) {
      int k;

      DEBUG(7,fprintf(stderr,"wikiwiki_end_markup: line='%s', sep='%s', i=%d, line[i]=%c\n",
		      line.c_str(),sep,i,line[i]
		      ));

      for(k=i+1;k<N && is_sep(line[k]) && !is_space(line[k])  && !is_same(line[k],sep[0]);k++);
      if (k==N || is_space(line[k])) {
	return 1;
      }
      else {
	return 0;
      }
    }
    else {
      return 0;
    }
  }
  else {
    return 0;
  }
}

std::string spod_parser::parse_wikiwikiwiki_recurse(std::string & line,std::string & end_sep,std::string link)
{
  int i=0,N=line.length();
  std::string subline;
  std::string new_sep;

  DEBUG(6,fprintf(stderr,"wikiwikiwiki_recurse: '%s' (%s,%s)\n",line.c_str(),end_sep.c_str(),link.c_str()));

  for(i=0;i<N &&
	!is_wikiwiki_start_markup(i,line,"*") &&
	!is_wikiwiki_start_markup(i,line,"/") &&
	!is_wikiwiki_start_markup(i,line,"_") &&
	!is_wikiwiki_start_markup(i,line,"#") &&
	!is_wikiwiki_start_markup(i,line,"[") &&
	!is_wikiwiki_end_markup(i,line,end_sep.substr(0,1).c_str())
	;i++);

  if (i<N) {

    subline=line.substr(0,i);

    if (is_wikiwiki_end_markup(i,line,end_sep.substr(0,1).c_str())) {
      std::string sep=end_sep.substr(0,1);
      end_sep=end_sep.substr(1);

      DEBUG(6,fprintf(stderr,"i=%d, line[i]=%c orig line='%s', link='%s', subline='%s'\n",i,line[i],line.c_str(),link.c_str(),subline.c_str()));

      line=line.substr(i+is_wikiwiki_end_markup(i,line,sep.c_str()));

      DEBUG(6,fprintf(stderr,"new line='%s'\n",line.c_str()))


      if (sep=="]") {
	if (link=="##@IMG@##") {
	  return parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	    parse_wikiwikiwiki_recurse(line,end_sep,"");
	}
	else {
	  std::string s;

	  if (subline=="") { subline=link; }
	  s=parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	    "|"+link+"> "+
	    parse_wikiwikiwiki_recurse(line,end_sep,"");

	  DEBUG(6,fprintf(stderr,"s=%s\n",s.c_str()));

	  return s;
	}
      }
      else if (sep=="#") {
	return
	  parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	  ">> "+
	  parse_wikiwikiwiki_recurse(line,end_sep,link);
      }
      else {
	return
	  parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	  "> "+
	  parse_wikiwikiwiki_recurse(line,end_sep,link);
      }
    }
    else if (is_wikiwiki_start_markup(i,line,"#")) {
      end_sep="#"+end_sep;
      line=line.substr(i+is_wikiwiki_start_markup(i,line,"#"));

      return
	parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	"S<C<"+
	parse_wikiwikiwiki_recurse(line,end_sep,link);
    }
    else if (is_wikiwiki_start_markup(i,line,"*")) {
      end_sep="*"+end_sep;
      line=line.substr(i+is_wikiwiki_start_markup(i,line,"*"));

      return
	parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	"B<"+
	parse_wikiwikiwiki_recurse(line,end_sep,link);
    }
    else if (is_wikiwiki_start_markup(i,line,"/")) {
      end_sep="/"+end_sep;
      line=line.substr(i+is_wikiwiki_start_markup(i,line,"/"));

      return
	parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	"I<"+
	parse_wikiwikiwiki_recurse(line,end_sep,link);
    }
    else if (is_wikiwiki_start_markup(i,line,"_")) {
      end_sep="_"+end_sep;
      line=line.substr(i+is_wikiwiki_start_markup(i,line,"_"));

      return
	parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	" U<"+
	parse_wikiwikiwiki_recurse(line,end_sep,link);
    }
    else if (is_wikiwiki_start_markup(i,line,"[")) {
      std::string word,img_type,img_word;
      int k;
      int C=is_wikiwiki_start_markup(i,line,"[");

      for(k=i+C;k<N && !isspace(line[k]) && line[k]!=']';k++);

      word=line.substr(i+C,k-(i+C)); //+1);
      line=line.substr(k);
      img_word=word;

      end_sep="]"+end_sep;

      if (is_image_word(img_word,img_type)) {   /* SIDE EFFECTS! */
	std::string s;

	s=parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	  "\n\n=image "+img_word+","+img_word+","+img_word+","+img_type+",center\n\n"+
	  parse_wikiwikiwiki_recurse(line,end_sep,"##@IMG@##");

	DEBUG(7,fprintf(stderr,"image:'%s'\n",s.c_str()));

	return s;
      }
      else {
	return
	  parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	  "L<"+
	  parse_wikiwikiwiki_recurse(line,end_sep,word);
      }
    }
    else {
      return
	parse_wikiwikiwiki_recurse(subline,new_sep,"")+
	parse_wikiwikiwiki_recurse(line,end_sep,link);
    }
  }
  else { /* No special things found, search for wikiwiki words */
    std::string newsub;
    int  b=0,e=0;

    for(i=0;i<N;i++) {
      e=i;
      if (isspace(line[i])) {
	if (e>b) {
	  std::string word=line.substr(b,e-b);
	  std::string w,r;
	  if (is_wikiwikiword(word,w,r)) {
	    DEBUG(6,fprintf(stderr,"word=%s, w=%s, r=%s\n",word.c_str(),w.c_str(),r.c_str()));
	    newsub+="L<";
	    newsub+=w;
	    newsub+="|";
	    newsub+=w;
	    newsub+=">";
	    newsub+=r;
	  }
	  else if (is_url(word)) {
	    newsub+="L<";
	    newsub+=word;
	    newsub+="|";
	    newsub+=word;
	    newsub+=">";
	  }
	  else {
	    newsub+=wikiwiki_escape(word);
	  }
	  newsub+=line[i];
	  b=i+1;
	}
	else {
	  newsub+=line[i];
	  e=b=i+1;
	}
      }
    }

    if (b<e) {
      newsub+=line.substr(b,e-b+1);
    }

    DEBUG(6,fprintf(stderr,"returns %d,%d %s\n",b,e,newsub.c_str()));

    return newsub;
  }
}

static
std::string create_table_code(bool in_table,bool is_table,std::string text)
{
  if (in_table && !is_table) {
    return "\n\n=table\n\n"+text;
  }
  else if (is_table) {
    int i,N;
    std::string nt;
    for(i=0,N=text.length();i<N;i++) {
      if (text[i]=='|') {
	const char *c="=cell";

	if (i<N-2) {
	  if (isspace(text[i+2])) {
	    if (text[i+1]=='r') {
	      c="=rcell";
	      i+=1;
	    }
	    else if (text[i+1]=='c') {
	      c="=ccell";
	      i+=1;
	    }
	  }
	}

	nt+="\n\n";
	nt+= c;
	nt+=" ";
      }
      else {
	nt+=text[i];
      }
    }
    return nt;
  }
  else {
    return text;
  }
}

std::string spod_parser::parse_wikiwikiwiki_line(std::string & line,int & list_level)
{
  int L=line.length();
  std::string new_line;
  bool items=false;
  bool in_table=is_table;

  DEBUG(5,fprintf(stderr,"wikiwikiwiki_line: '%s'\n",line.c_str()));

  is_table=false;

  if (L>=6 && line.substr(0,6)=="======") {
    new_line="=head6 ";
    line=line.substr(6);
  }
  else if (L>=5 && line.substr(0,5)=="=====") {
    new_line="=head5 ";
    line=line.substr(5);
  }
  else if (L>=4 && line.substr(0,4)=="====") {
    new_line="=head4 ";
    line=line.substr(4);
  }
  else if (L>=3 && line.substr(0,3)=="===") {
    new_line="=head3 ";
    line=line.substr(3);
  }
  else if (L>=2 && line.substr(0,2)=="==") {
    new_line="=head2 ";
    line=line.substr(2);
  }
  else if (L>=1 && line.substr(0,1)=="=") {
    new_line="=head1 ";
    line=line.substr(1);
  }
  else if (L>=4 && line.substr(0,4)=="----") {
    new_line="=hline ";
    line=line.substr(4);
  }
  else if (L>=1 && line.substr(0,1)=="|") {
    if (!in_table) {
      int i,pipes=0;
      for(i=0;i<L;i++) {
	if (line[i]=='|') { pipes+=1; }
      }
      {
	new_line="=table ";
	for(i=0;i<pipes;i++) {
	  new_line+=" 0";
	}
	new_line+=",,,1";
      }
      new_line+="\n\n";
    }
    else {
      new_line="\n=row";
    }
    is_table=true;
  }
  else if (L>=1 && line.substr(0,1)=="*") {
    int i;
    char buf[100];

    for(i=0;i<L && line.substr(i,1)=="*";i++);
    if (line.substr(i,1)==" ") {
      if (i>list_level) {
	sprintf(buf,"\n=over %d\n\n=item *\n\n",i-list_level);
      }
      else if (i<list_level) {
	sprintf(buf,"\n=back %d\n\n=item *\n\n",list_level-i);
      }
      else {
	sprintf(buf,"\n=item *\n\n");
      }
      new_line=buf;

      list_level=i;
      for(;i<L && isspace(line[i]);i++);
      line=line.substr(i);

      items=true;
    }
  }
  else if (L>=1 && line.substr(0,1)=="0") {
    int i;
    char buf[100];

    for(i=0;i<L && line.substr(i,1)=="0";i++);
    if (line.substr(i,1)==" ") {
      if (i>list_level) {
	sprintf(buf,"\n=over %d\n\n=item 0\n\n",i-list_level);
      }
      else if (i<list_level) {
	sprintf(buf,"\n=back %d\n\n=item 0\n\n",list_level-i);
      }
      else {
	sprintf(buf,"\n=item 0\n\n");
      }
      new_line=buf;

      list_level=i;
      for(;i<L && isspace(line[i]);i++);
      line=line.substr(i);

      items=true;
    }
  }

  if (!items && list_level>0) {
    char buf[100];
    sprintf(buf,"\n=back %d\n\n",list_level);
    list_level=0;
    new_line=buf+new_line;
  }

  {
    std::string R;

    if (line.substr(0,1)==" " || line.substr(0,1)=="\t") {
      if (!is_verbatim_text) {
	is_verbatim_text=true;
	new_line=new_line+"\n";
      }
      R=new_line+line;
    }
    else {
      std::string end_sep;

      if (is_verbatim_text) {
	new_line=new_line+"\n";
	is_verbatim_text=false;
      }

      R=remove_wikiwiki_escaped_markup(new_line+
				       create_table_code(in_table,is_table,parse_wikiwikiwiki_recurse(line,end_sep,""))
				       );
    }

    return R;
  }
}


std::string spod_parser::parse_wikiwikiwiki_block(std::string & block)
{
  std::string body;
  int         i,position=0,N=block.length(),L;
  bool        eof_block=(N==0);
  std::string line;
  int         list_level=0;

  is_verbatim_text=false;
  is_table=false;

  while(!eof_block) {
    for(i=position,L=0;i!=N && !(L=eof_line(block,i));i++);
    {
      int E=i+L;
      DEBUG(6,fprintf(stderr,"position=%d,E=%d,len=%d\n",position,E,E-position+1));
      line=block.substr(position,E-position);
      body+=parse_wikiwikiwiki_line(line,list_level);
      position=E;
    }
    eof_block=position==N;
  }

  DEBUG(5,fprintf(stderr,"BODY!\n%s\n",body.c_str()));

  return body;
}

void spod_parser::parse_wikiwikiwiki(std::string & body)
{
  std::string directive,block;
  bool        tmp;
  std::string original_block;

  while(read_block(directive,block,tmp,original_block)) {
    if (directive=="=wikiwikiwiki") {
      body+=parse_wikiwikiwiki_block(block);
    }
    else {
      body+=original_block;
    }
  }

  DEBUG(3,fprintf(stderr,"parse_wikiwikiwiki::body:\n%s\n",body.c_str()));
}


void spod_parser::parse_body(std::string & body)
{
  std::string directive,line,original_line;
  bool	 	goon=true;
  bool		has_reread=false;

  DEBUG(5,fprintf(stderr,"starting body size=%ld, _ispod=%d\n",body.size(),_ispod));

  while(goon && read_block(directive,line,has_reread,original_line)) {

    if (directive=="=cut") {
      _ispod=false;
      parse_directive(directive,line,body);
    }
    else if (directive!="=p" && directive!="=pre" && directive!="=pod" && directive!="=verbatim") {
      _ispod=true;
    }
    else if (directive=="=pod") {
      // do nothing now, from next line text is pod
    }

    DEBUG(5,fprintf(stderr,"parse_body:%d %s %s\n",_ispod,directive.c_str(),line.c_str()));

    if (_ispod && directive!="=skip") {
      if (_next_block_is_title) {
	_title=ltrim(line);
	_next_block_is_title=false;
      }
      parse_head(directive,line,has_reread);

      // Get the verbatim part going.

      if (directive=="=verbatim") {
	std::string syn="txt,8";
	int         i;
	i=line.find("\n");
	if (i!=-1) {
	  syn=line.substr(0,i);
	  line=line.substr(i+1);
	}
	parse_directive("=syn",syn,body);
	parse_directive("=pre",line,body);
	goon=parse_directive("=p","",body);
      }
      else {
	goon=parse_directive(directive,line,body);
      }

    }
    else {
      goon=true;
    }

    if (directive=="=pod") { _ispod=true; }
  }
}

std::string spod_parser::link_id(const std::string & text)
{
  std::string id=text;

  trim(id);

  {
    int i;
    int N=id.length();

    for(i=0;i<N;i++) {
      if (isspace(id[i])) {
	id[i]='_';
      }
      else if (!(isalpha(id[i]) || isdigit(id[i]))) {
	id[i]='_';
      }
    }
  }

  return tolower(id);
}

void spod_parser::parse_head(const std::string & directive, const std::string & line,bool has_reread)
{
  if (!has_reread) {
    if (directive.substr(0,5)=="=head") {
      int 		level=to_int(directive.substr(5));
      std::string ln=ltrim(line);
      _toc.insert(_toc.end(),ln);
      _toc_level.insert(_toc_level.end(),level);
      _toc_id.insert(_toc_id.end(),link_id(line));

      if (tolower(ln)=="name") { _next_block_is_title=true; }
    }
  }
}

void spod_parser::rewind(void)
{
  _cursor=0;
  _length=_in.size();
  _line_number=0;
}

bool spod_parser::read_block(std::string & directive,std::string & block,bool & has_reread,std::string & original_block)
{
  bool result;

  if (_reread) {
    block=_last_block;
    directive=_last_directive;
    original_block=_last_original_block;
    result=true;
    _reread=false;
    has_reread=true;
  }
  else {
    std::string line,oline;
    std::string dir;
    if (read_line(dir,line,has_reread,oline)) {
      bool ok;
      block=line;
      original_block=oline;
      directive=dir;
      if (directive=="=verbatim") {
	while((ok=read_line(dir,line,has_reread,oline)) && dir!="=verbatim") {
	  DEBUG(6,fprintf(stderr,"read_line: directive=%s\n",dir.c_str()));
	  block+=oline;
	  original_block+=oline;
	}
	if (ok) {
	  original_block+=oline;
	}
      }
      else if (directive=="=wikiwikiwiki") {
	while((ok=read_line(dir,line,has_reread,oline)) && dir!="=wikiwikiwiki") {
	  DEBUG(6,fprintf(stderr,"read_line: directive=%s\n",dir.c_str()));
	  block+=oline;
	  original_block+=oline;
	}
	if (ok) {
	  original_block+=oline;
	}
      }
      else {
	if (ltrim(line)!="") {
	  while((ok=read_line(dir,line,has_reread,oline)) && ltrim(line)!="" && dir==directive) {
	    block+=line;
	    original_block+=oline;
	  }
	  if (ok) {
	    original_block+=oline;
	  }
	}

      }
      result=true;
      _last_block=block;
      _last_directive=directive;
      _last_original_block=original_block;
    }
    else {
      result=false;
      _last_block="";
      _last_directive="";
      _last_original_block="";
    }
  }

  DEBUG(5,fprintf(stderr,"read_block:%s,%s\n",directive.c_str(),block.c_str()));
  return result;
}

bool spod_parser::read_line(std::string & directive,std::string & line,bool & has_reread,std::string & original_line)
{
  bool result;

  // read line

  if (_cursor==_length) {
    result=false;
    line="";
    has_reread=false;
  }
  else {
    int _from=_cursor;
    while(_cursor<_length && _in[_cursor]!='\n') { _cursor+=1; }
    DEBUG(8,fprintf(stderr,"%d,%d,%d\n",_from,_cursor,_length));
    _cursor+=1;
    line=_in.substr(_from,_cursor-_from);
    has_reread=false;
    result=true;
    _line_number+=1;
  }

  original_line=line;

  // Parse for a directive

  {int i;
  std::string prev_directive=directive,comment;

  directive="";

  // skip comments

  if (_comments.size()>0) {int pos;

  for(pos=0;pos<(int) line.size() && isspace(line[pos]);pos++);
  if (pos!=(int) line.size()) {int c;

  for(c=0;c<(int) _comments.size() && line.substr(pos,_comments[c].size())!=_comments[c];c++);
  if (c!=(int) _comments.size()) {
    DEBUG(7,fprintf(stderr,"line[%d]=%s\n",pos,line.substr(pos).c_str()));
    int size=_comments[c].size();
    for(pos+=size;pos<(int) line.size() && line.substr(pos,size)==_comments[c];pos+=size);
    line=line.substr(pos);
  }
  }
  }

  // Get directive

  DEBUG(7,fprintf(stderr,"line=%s\n",line.c_str()));

  if (directive=="") {
    if (isspace(line[0]) && !linefeed_only(line)) {
      directive="=pre";
    }
    else {
      if (line[0]=='=') {
	for(i=0;i<(int) line.size() && !isspace(line[i]);i++);
	directive=line.substr(0,i);
	for(;i<(int) line.size() && isspace(line[i]);i++);
	line=line.substr(i,line.size());
      }
      else {
	directive="=p";
      }
    }
  }
  }

  return result;
}

bool spod_parser::is_verbatim(const std::string & block)
{
  DEBUG(3,fprintf(stderr,"=verbatim: %s!=%s?\n",tolower(ltrim(block)).c_str(),"end"));
  return tolower(ltrim(block))!="end";
}


void spod_parser::get_parameters(const std::string & block,std::string & parameters,std::string & newblock)
{
  int i;
  i=block.find("\n");
  if (i!=-1) {
    parameters=ltrim(block.substr(0,i));
    newblock=block.substr(i+1);
  }
}

void spod_parser::push_back_block(void)
{
  _reread=true;
}

std::string & spod_parser::body(void)
{
  return _body;
}

std::string & spod_parser::title(void)
{
  if (_title=="") {
    if (_toc.size()>0) {
      _title=_toc[0];
    }
    else {
      _title="unknown";
    }
  }
  return _title;
}

bool spod_parser::current_toc(int & level,std::string & head,std::string & id)
{
  return toc(_toc.size()-1,level,head,id);
}

bool spod_parser::toc(int i,int & level,std::string & head,std::string & id)
{
  if (i>=(int) _toc.size()) {
    level=-1;
    head="";
    id="";
    return false;
  }
  else {
    level=_toc_level[i];
    head=_toc[i];
    id=_toc_id[i];
    return true;
  }
}

void spod_parser::trim(std::string & line)
{
  int i,k;
  for(i=0;i<(int) line.size() && isspace(line[i]);i++);
  for(k=line.size()-1;k>=i && isspace(line[k]);k--);
  line=line.substr(i,k+1);
}

std::string spod_parser::ltrim(const std::string & line)
{
  std::string s(line);
  trim(s);
  return s;
}

std::string spod_parser::tolower(const std::string & s)
{
  int i,N;
  std::string S=s;

  for(i=0,N=S.size();i<N;i++) {
    if (S[i]>='A' && S[i]<='Z') { S[i]=::tolower(S[i]); }
  }
  return S;
}

bool spod_parser::is_empty(const std::string & line)
{
  int i;
  for(i=0;i<(int) line.size() && isspace(line[i]);i++);
  return i==(int) line.size();
}

int spod_parser::to_int(std::string line)
{
  trim(line);
  return atoi(line.c_str());
}

bool spod_parser::linefeed_only(const std::string & line)
{
  return line[0]=='\n';
}

void spod_parser::split(const std::string & s,
		   const std::string & substr,
		   std::vector<std::string> & parts)
{
  int from,to,size;
  parts.clear();
  from=0;
  to=-1;
  size=substr.size();
  while((to=s.find(substr,from))!=-1) {
    if (to==-1) {
      parts.insert(parts.end(),s.substr(from));
    }
    else {
      parts.insert(parts.end(),s.substr(from,to-from));
    }
    from=to+size;
  }
  parts.insert(parts.end(),s.substr(from));
}

/***************************************************************************************/

bool spod_parser::is_uint(const std::string & s)
{
  int i;
  for(i=0;i<(int) s.size() && s[i]>='0' && s[i]<='9';i++);
  if (i==(int) s.size()) {
    return true;
  }
  else if (isspace(s[i])) {
    return true;
  }
  else {
    return false;
  }
}

void spod_parser::warning(const char * format, ...) {
  va_list ap;
  va_start(ap,format);
  fprintf(stderr,"block before line %d: ",_line_number);
  vfprintf(stderr,format,ap);
  fprintf(stderr,"\n");
  va_end(ap);
}
