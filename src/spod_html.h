#ifndef __SPOD_HTML__H
#define __SPOD_HTML__H

// $Id: spod_html.h,v 1.13 2005/08/02 08:27:25 cvs Exp $ 

#include <spod_parser.h>
#include <harray.h>
#include <string>
#include <stack>

#define CT_NORMAL 0
#define CT_HEADER 1

class spod_html : public spod_parser
{
 private:
  std::string 		_prev_directive;
  std::stack<int>	_head_levels;
  std::stack<int>	_item_levels;
  bool			_in_item_sequence;
  std::string		_item_type;
  bool			_in_pre_sequence;
  bool			_in_p_sequence;
  bool			_have_enscript;
  std::string		_tempfile;
  std::string		_pre;
  std::string 		_link_prefix,_link_postfix;
  std::vector<int>      _cell_widths;
  bool                  _in_table;
  int                   _table_cell,_table_row,_cell_type;
 public:
  spod_html(int fh,const std::string & comment="",const std::string & prefix="",const std::string & postfix="");
  ~spod_html();
 public:
  std::string parse_escape     (const std::string & escape_type,const std::string & body);
  bool        parse_directive  (const std::string & directive,const std::string & line,std::string & body);
  void        parse_init       (void);
  void        parse_finish     (std::string & body);
 public:
  std::string escape           (const std::string & block);
 protected:
  virtual std::string generate_link(std::string & type,std::string & link, std::string & internal_link,std::string & description);
};

#endif
