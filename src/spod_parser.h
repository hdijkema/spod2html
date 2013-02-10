#ifndef __SPOD__H
#define __SPOD__H

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include <vector>

class spod_parser {
 private:
  std::string 		        _body;
  std::vector<std::string>	_comments;
  std::string 		        _last_block,
                                _last_directive,
                                _last_original_block;
  bool				_reread;
  bool				_ispod;
  std::string                   _in;
  int				_cursor,_length;
  std::string			_title;
  std::vector<std::string> 	_toc;
  std::vector<std::string>	_toc_id;
  std::vector<int>		_toc_level;
  int				_debug;
  int				_line_number;
  std::stack<std::string>	_parts;
 private:		
  std::string			_syntax;
  int				_tabsize;
  bool				_next_block_is_title;
 public:
  // Construction/Destruction
  spod_parser(int fh,const std::string & comment="");
  virtual ~spod_parser();

 public:
  void 			debug                (int level=0)   { _debug=level; }
 protected:
  int			debug_level          (void)	     { return _debug; }

 public:
  virtual bool 		parse_directive      (const std::string & directive,const std::string & line,std::string & body) = 0;
  virtual void 		parse_init           (void) = 0;
  virtual void		parse_finish         (std::string & body) = 0;
  virtual std::string	parse_escape         (const std::string & escape, const std::string & part) = 0;

  // WikiWikiWiki Mode
 private:
  bool is_verbatim_text;
  bool is_table;
 public:
  std::string parse_wikiwikiwiki_line        (std::string & line,int & list_level);
  std::string parse_wikiwikiwiki_block       (std::string & block);
  std::string parse_wikiwikiwiki_recurse     (std::string & line,std::string & end_sep,std::string link);
  int         is_wikiwiki_start_markup       (int i,std::string line,const char *sep);
  int         is_wikiwiki_end_markup         (int i,std::string line,const char *sep);
  bool        is_image_word                  (std::string & word,std::string & type);

  // Escapes
 public: 
  void parse(void);
 protected:
  void          parse_body             (std::string & body);
  void		parse_escapes          (std::string & body);
  void          parse_wikiwikiwiki     (std::string & body);

  // Parsing
 protected:
  bool          parse_image            (std::string block,
					std::string & img_descr,      
					std::string & img_name,
					std::string & img_url,
					std::string & img_type,
					std::string & img_align);
bool            parse_table            (std::string block,
			                std::vector<int> & widths,
			                std::string & caption,
			                int & border,
			                int & frame);
  bool		parse_link             (const std::string & link, std::vector<std::string> & link_parts);
  std::string   link_id                (const std::string & reference);
  std::string   get_item_type          (const std::string & block);

 private:
  std::string   parse_escapes_for_part (const std::string & part,int level);
  bool          read_line              (std::string & directive,std::string & line,bool & has_reread,std::string & original_line);

  // Reading
 protected:
  void		rewind                 (void);
  bool 		read_block             (std::string & directive,std::string & block,bool & has_reread,std::string & original_block);
  void		get_parameters         (const std::string & block,std::string & parameters, std::string & newblock);
  void		push_back_block        (void);
  void		parse_head             (const std::string & directive, const std::string & line,bool has_reread);

 public:
  std::string & body                   (void);
  std::string & title                  (void);
  bool          toc                    (int i,int & level,std::string & head,std::string & id);
  bool		current_toc            (int & level,std::string & head,std::string & id);

 public:
  std::string & syntax                 (void) 			 { return _syntax; }
  void		set_syntax             (std::string s)           { _syntax=s; }
  int		tabsize                (void)			 { return _tabsize; }
  void		set_tabsize            (int t)		         { _tabsize=t; }
  bool		is_verbatim            (const std::string & s);

 public:
  void          warning                (const char * format,...);

 public:
  void          trim                   (std::string & line);
  std::string   ltrim                  (const std::string & line);
  std::string   tolower                (const std::string & s);
  bool          is_empty               (const std::string & line);
  int           to_int                 (std::string str);
  bool          linefeed_only          (const std::string & line);
  void          split                  (const std::string & s,const std::string & character,std::vector<std::string> & parts);
  bool          is_uint                (const std::string & s);
};

#define SPODDBG(level,a)		if (this->debug_level()>=level) { \
									fprintf(stderr,"DEBUG(%d):",level); \
									a; \
									fseek(stderr,0,SEEK_END); \
								}



#endif
