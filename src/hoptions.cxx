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

#include <hoptions.h>
#include <stdio.h>
#include <stdlib.h>

#define debug(a) //fprintf(stderr,"%s\n",a);fseek(stderr,0,SEEK_END)

hoptions::hoptions(int argc,char *argv[],const std::string & ko)
{
	init(argc,(const char **) argv,ko);
}

hoptions::hoptions(int argc,const char *argv[],const std::string & known_options)
{
	init(argc,argv,known_options);
}

	
void hoptions::init (int argc,const char *argv[],const std::string & known_options)
{
std::vector<std::string> known;	
std::string s=trim(known_options);
int i;

	// Get known options
	
	debug("getting known options");
	
	while(s!="") {
		int k=s.find("|");
		if (k==-1) {
			known.insert(known.end(),s);
			s="";
		}
		else {
			std::string opt=trim(s.substr(0,k));
			debug(opt.c_str());
			s=s.substr(k+1);
			debug(s.c_str());
			if (opt!="") {
				known.insert(known.end(),opt);
			}
		}
	}
	
	_known=known;
	debug("known options ok");
	
	
	// Get options
	
	_ok=true;
	
	for(i=1;i<argc;i++) {
		std::string s=argv[i],key,value;
		int         k;

		if (s.size()>=2 && s.substr(0,2)=="--") {
				s=s.substr(2);
				
				k=s.find("=");
				if (k==-1) {
					key=s;
					value="";
				}
				else {
					key=s.substr(0,k);
					value=s.substr(k+1);
				}
				
				key=trim(key);
				value=trim(value);
				
				if (_ok) {int k;
					for(k=0;k<((int) known.size()) && known[k]!=key;k++);
					if (k==((int) known.size())) {
						_ok=false;
						_error_msg="Unknown option '";
						_error_msg+=key;
						_error_msg+="'";
					}
				}
				
				_keys.insert(_keys.end(),key);
				_values.insert(_values.end(),value);
		}
		else {
			debug("inserting");
			debug(s.c_str());
			insert(end(),s);
		}
	}
	
	debug("getting options ok");
}


void hoptions::info(void)
{
int i;
	fprintf(stderr,"Known options:\n");
	for(i=0;i<(int) _known.size();i++) {
		fprintf(stderr,"  --%s\n",_known[i].c_str());
	}
	fprintf(stderr,"Given options:\n");
	for(i=0;i<(int) _keys.size();i++) {
		fprintf(stderr,"  --%s=%s\n",_keys[i].c_str(),_values[i].c_str());
	}
	fprintf(stderr,"Status: %d (%s)\n",ok(),error().c_str());
}

std::string hoptions::trim(const std::string & line)
{
int i,k;
	for(i=0;i<((int) line.size()) && isspace(line[i]);i++);
	for(k=line.size()-1;k>=i && isspace(line[k]);k--);
return line.substr(i,k+1);
}


bool hoptions::ok(void)
{
	return _ok;
}

const std::string & hoptions::error(void)
{
	return _error_msg;
}

bool hoptions::get(const std::string & key,std::string & value)
{
int k;
	for(k=0;k<((int) _keys.size()) && _keys[k]!=key;k++);
	if (k==((int) _keys.size())) {
		value="";
		return false;
		
	}
	else {
		value=_values[k];
		return true;
	}
}

bool hoptions::get(const std::string & key) 
{
std::string v;
	return get(key,v);
}

bool hoptions::get(const std::string & key,int & value)
{
std::string v;
	if (get(key,v)) {
		value=atoi(v.c_str());
		return true;
	}
	else {
		value=0;
		return false;
	}
}

bool hoptions::get(const std::string & key,unsigned int & value)
{
int v;
	if (get(key,v)) {
		value=(unsigned int) v;
		return true;
	}
	else {
		value=0;
		return false;
	}
}

bool hoptions::get(const std::string & key,double & value)
{
std::string v;
	if (get(key,v)) {
		value=atof(v.c_str());
		return true;
	}
	else {
		value=0.0;
		return false;
	}
}

