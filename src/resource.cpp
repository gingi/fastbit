//$Id$
//Author: John Wu <John.Wu@ACM.org>
//        Lawrence Berkeley National Laboratory
//Copyright 2000-2008 the Regents of the University of California
//
// Purpose: The implementation of class ibis::resource
//
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "util.h"
#include "resource.h"	// class ibis::resource
#include <stdlib.h>	// getenv

// delimiters allowed in the resource name string
const char* ibis::resource::delimiters = "*:.";

/// Read a configuration file.
/// It will read the first file in the following list and add the content
/// to the existing list of parameter,
/// - (1) argument to this function (fn),
/// - (2) environment variable IBISRC,
/// - (3) file named ibis.rc in the current directory,
/// - (4) file named .ibisrc in the current directory,
/// - (5) file named .ibisrc in the user's home directory.
///
/// It will attempt to parse the content of the first file it finds.  The
/// content of the file is parsed and added to the current content of the
/// resouce object.  The parameters with the same will overwrite the existing
/// values.
/// If it can not find any one of the files, it will return without
/// modifying the current content of the resource object.
void ibis::resource::read(const char* fn) {
    char line[MAX_LINE];
    FILE* conf = 0;
    std::string tried;
    const char* name = fn; // first choice is the argument to this function
    if (name != 0 && *name != 0 && ibis::util::getFileSize(name) > 0) {
	tried = name;
	conf = fopen(name, "r");

	if (conf == 0){
	    LOGGER(0)
		<< "Warning -- ibis::resource::read failed to open user "
		"specified file \"" << name << "\" ... "
		<< (errno ? strerror(errno) : "no free stdio stream");
	    return;
	}
    }
    if (conf == 0) {
	// second choice is the environment variable
	name = getenv("IBISRC");
	if (name != 0 && *name != 0 && ibis::util::getFileSize(name) > 0) {
	    conf = fopen(name, "r");
	    if (tried.empty())
		tried = name;
	    else {
		tried += "\n";
		tried += name;
	    }
	}
    }
    if (conf == 0) {
	// thrid choice is a file in this directory
	name = "ibis.rc";
	conf = fopen(name, "r");
	if (tried.empty())
	    tried = name;
	else {
	    tried += "\n";
	    tried += name;
	}
    }
    if (conf == 0) {
	// fourth choice is a hidden file in this directory
	name = ".ibisrc";
	conf = fopen(name, "r");
	if (tried.empty())
	    tried = name;
	else {
	    tried += "\n";
	    tried += name;
	}
    }
    if (conf == 0) {
	// the last choice is .ibisrc in user's home directory
	name = getenv("HOME");
	if (name != 0 && *name != 0) {
#if defined(sun) && defined(__GNUC__) && __GNUC__ <= 2
	    long ierr = sprintf(line, "%s%c.ibisrc", name, DIRSEP);
#else
	    long ierr = UnixSnprintf(line, MAX_LINE, "%s%c.ibisrc",
				     name, DIRSEP);
#endif
	    if (ierr > 0 && ierr < MAX_LINE &&
		ibis::util::getFileSize(line) > 0) {
		conf = fopen(line, "r");
		name = line;
		if (tried.empty())
		    tried = line;
		else {
		    tried += "\n";
		    tried += line;
		}
	    }
	}
    }
    if (0 == conf) {
	LOGGER(4) << "ibis::resource::read -- can not find any of the "
	    "following configuration files:\n" << tried.c_str();
	return;
    }

    char *value;
    LOGGER(1) << "ibis::resource::read -- Reading configuration file \""
	      << (name?name:"") << "\""; 
    while ( !feof(conf) ) {
	if (fgets(line, MAX_LINE, conf) == 0) continue; // skip empty line

	// '!' and '#' denotes comment
	if ( !line[0] || line[0]=='\n' || line[0]=='\r' ||
	     line[0]=='!' || line[0]=='#' )
	    continue; // skip comment line

	char* tmp = line + (strlen(line) - 1);
	*tmp = static_cast<char>(0); // get rid of the '\n'
	--tmp;
	while (isspace(*tmp) && tmp>=line) { // remove trailing blanks
	    *tmp = static_cast<char>(0); -- tmp;
	}
	if (tmp <= line) continue; // empty line (or a single character)

	name = line;
	value = strchr(line, '=');
	if (value) {
	    *value = static_cast<char>(0); // terminate name string
	    ++value;
	    add(name, ibis::util::trim(value));
	}
	else {
	    LOGGER(7) << "ibis::resource::read -- skipping line \""
		      << line << "\" because it contains no '='";
	}
    }
    fclose(conf);
#ifdef DEBUG
    ibis::util::logger lg(ibis::gVerbose);
    write(lg.buffer());
#endif
} // ibis::resource::read

/// Add a name-value pair to the resource list.  It replaces the existing
/// value.
void ibis::resource::add(const char* name, const char* value) {
    char* mname = ibis::util::strnewdup(name);
    char* tname = ibis::util::trim(mname); // remove surronding spaces
    tname += strspn(tname, delimiters); // skip repeated delimiters
    char* tmp = strpbrk(tname, delimiters);
    if (tmp == 0) { // add parameter at this level
	// erase the old content
	ibis::resource::vList::iterator vit = values.find(tname);
	if (vit != values.end()) { // free the old value
	    delete [] (*vit).second;
	    (*vit).second = 0;
	    if (value != 0 && *value != 0)
		(*vit).second = ibis::util::strnewdup(value);
	}
	else { // a new name-value pair
	    values[ibis::util::strnewdup(tname)] =
		ibis::util::strnewdup(value);
	}
	ibis::resource::gList::iterator it = groups.find(tname);
	if (it != groups.end()) { // erase the group with the same name
	    delete (*it).second;
	    groups.erase(it);
	}
    }
    else { // involving another level ?
	*tmp = static_cast<char>(0); ++ tmp;
	tname = ibis::util::trim(tname);
	if (*tname == 0 || (context == 0 &&
			    (stricmp(tname, "common") == 0 ||
			     stricmp(tname, "all") == 0))) {
	    add(tmp, value); // add to this level
	}
	else { // need to look for the named group
	    ibis::resource::vList::iterator vit = values.find(tname);
	    if (vit != values.end()) {// erase the named parameter
		delete [] (char*)((*vit).first);
		delete [] (*vit).second;
		values.erase(vit);
	    }
	    ibis::resource::gList::iterator it = groups.find(tname);
	    if (it != groups.end()) { // add it to the group
		(*it).second->add(tmp, value);
	    }
	    else { // need to allocate a new group with the specified name
		resource* res = new resource(this, tname);
		groups[res->prefix] = res;
		res->add(tmp, value);
	    }
	}
    }
    delete [] mname;
} // ibis::resource::add

// Return a value for the named parameter -- if the name does not appear
// in the specified context, it will search for the parameter in the
// lowest level of context containing the named one.
const char* ibis::resource::operator[](const char* name) const {
    const char* value = static_cast<char*>(0);
    if (name==0) return value;
    if (*name==static_cast<char>(0)) return value;

    while (*name && isspace(*name)) ++name;
    if (*name == 0) return value;

    const char* tmp = strpbrk(name, delimiters);
    if (tmp == 0) { // no delimiter
	vList::const_iterator it = values.find(name);
	if (it != values.end())
	    value = (*it).second;
	else if (context) // search for the same name in higher levels
	    value = (*context)[name];
    }
    else {
	char *buf = ibis::util::strnewdup(name, tmp-name);
	const char *gname = ibis::util::trim(buf);
	tmp += strspn(tmp, delimiters); // skip consecutive delimiters
	gList::const_iterator itg = groups.find(gname);
	delete [] buf;

	if (itg != groups.end()) { // matched the prefix
	    value = (*((*itg).second))[tmp];
	}
	else { // search based on the trailing portion of the name
	    value = operator[](tmp);
	    if (value == 0 && context != 0)
		value = (*context)[tmp];
	}
    }
    return value;
} // ibis::resource::operator[](const char* name)

/// Parse the string value as a number.
/// If the first non-numeric character is a 'k' or 'm' or 'g', the proceeding
/// number is mutiplied by 1024, 1048576, or 1073742824.
/// If the first non-numeric character is 'h', the value before it
/// is multiplied by 3600 (h for hour), converting it from hours to seconds.
double ibis::resource::getNumber(const char* name) const {
    double sz = 0;
    const char* str = operator[](name);
    if (str != 0) {
	sz = atof(str);
	if (sz > 0) {
	    while (isspace(*str) || isdigit(*str) || *str == '.' ||
		   *str == 'e' || *str == 'E') ++str;
	    if (*str == 'k' || *str == 'K') sz *= 1024;
	    else if (*str == 'm' || *str == 'M') sz *= 1048576;
	    else if (*str == 'g' || *str == 'G') sz *= 1073742824;
	    else if (*str == 'h' || *str == 'H') sz *= 3600;
	}
    }
    return sz;
} // ibis::resource::getNumber

/// If the named parameter exists and its value is one of "true", "yes",
/// "on" or "1", this function will return true, otherwise false.
bool ibis::resource::isTrue(const char* name) const {
    const char* val = operator[](name);
    return isStringTrue(val);
} // ibis::resource::isTrue

/// Delete the content of the resource class.
void ibis::resource::clear() {
    delete [] (char*)prefix;
    for (vList::const_iterator it = values.begin();
	 it != values.end(); ++it) {
	delete[] (char*)((*it).first);
	delete[] (*it).second;
    }
    values.clear();

    for (gList::const_iterator git = groups.begin();
	 git != groups.end(); ++git)
	delete (*git).second;
    groups.clear();
} // ibis::resource::clear

/// Delete a simple list of name-value pairs.
void ibis::resource::clear(ibis::resource::vList &vl) {
    for (vList::const_iterator it = vl.begin();
	 it != vl.end(); ++it) {
	delete[] (char*)(*it).first;
	delete[] (*it).second;
    }
    vl.clear();
} // ibis::resource::clear

/// Parse a string of the form "name=vale, name=value, ..." into a simple
/// list of name-value pairs.  Add the new ones to the existing list.
void ibis::resource::parseNameValuePairs(const char *in,
					 ibis::resource::vList &lst) {
    if (in == 0) return;
    if (*in == 0) return;

    const char* str = in;
    const char* end;
    const char* tmp;
    while (*str != 0 && isspace(*str)) // skip leading space
	++ str;
    while ((tmp = strchr(str, '=')) != 0) {
	end = tmp;
	while (end > str && isspace(end[-1]))
	    -- end;
	if (end > str) { // found a name string
	    char *name = ibis::util::strnewdup(str, end-str);

	    str = tmp + 1;
	    while (*str && isspace(*str)) // skip space
		++ str;
	    if (*str) { // nonempty value string
		tmp = strchr(str, ',');
		if (tmp == 0)
		    tmp = strchr(str, ';');
		if (tmp == 0) {
		    end = strchr(str, '=');
		    if (end) {
			tmp = strchr(str, ' ');
			if (tmp == 0)
			    tmp = strchr(str, '\t');
		    }
		}
		if (tmp) {
		    end = tmp;
		    while (end > str && isspace(end[-1]))
			-- end;
		    if (end > str) {
			lst[name] = ibis::util::strnewdup(str, end-str);
		    }
		    else {
			lst[name] = ibis::util::strnewdup("*");
		    }
		    str = tmp + strspn(tmp,",; \t");
		}
		else {
		    lst[name] = ibis::util::strnewdup(str);
		    str += strlen(str);
		}
	    }
	    else  {
		lst[name] = ibis::util::strnewdup("*");
	    }
	} // if (end > str)
	else { // skip till next , or ;
	    str = tmp + 1;
	    if (*str) {
		tmp = strchr(str, ',');
		if (tmp == 0)
		    tmp = strchr(str, ';');
		if (tmp != 0)
		    str = tmp + strspn(tmp, ",; \t");
	    }
	}
    } // while ((tmp = strchr(str, '=')) != 0)
    if (ibis::gVerbose > 3) {
	ibis::util::logger lg(3);
	lg.buffer() << "ibis::resource::parseNameValuePairs() converted \""
		    << in << "\" into " << lst.size() << " name-value pairs";
	for (vList::const_iterator it = lst.begin();
	     it != lst.end(); ++ it)
	    lg.buffer() << "\n" << (*it).first << " = " << (*it).second;
    }
} // ibis::resource::parseNameValuePairs

/// The assignment operator.
const ibis::resource& ibis::resource::operator=(const ibis::resource& rhs) {
    if (&rhs == this) return *this;
    clear(); // clear the current content first
    groups = rhs.groups;
    values = rhs.values;
    context = rhs.context;
    if (prefix)	delete [] (char*)prefix;
    prefix = ibis::util::strnewdup(rhs.prefix);
    return *this;
} // ibis::resource::operator=

/// Write the content of this object to an output stream.
void ibis::resource::write(std::ostream& out, const char* ctx) const {
    out << "# begin parameters with ";
    if (prefix) {
	if (ctx)
	    out << ctx << "*";
	out << "prefix " << prefix << std::endl;
	for (vList::const_iterator vit = values.begin();
	     vit != values.end(); ++vit) {
	    if (ctx)
		out << ctx << "*";
	    out << prefix << "*" << (*vit).first << " = "
		<< (*vit).second << std::endl;
	}

	out << "# end parameters with prefix ";
	if (ctx)
	    out  << ctx << "*";
	out << prefix << std::endl;

	// write groups recursively
	char* tmp;
	if (ctx) {
	    tmp = new char[strlen(ctx)+strlen(prefix)+3];
	    strcpy(tmp, ctx);
	    strcat(tmp, "*");
	    strcat(tmp, prefix);
	}
	else if (prefix) {
	    tmp = ibis::util::strnewdup(prefix);
	}
	else {
	    tmp = new char[4];
	    tmp[0] = '*';
	    tmp[1] = 0;
	}
	for (gList::const_iterator git = groups.begin();
	     git != groups.end(); ++git)
	    (*git).second->write(out, tmp);
	delete [] tmp;
    }
    else {
	out << "global prefix" << std::endl;
	for (vList::const_iterator vit = values.begin();
	     vit != values.end(); ++vit)
	    out << (*vit).first << " = " << (*vit).second << std::endl;
	out << "# end parameters with global prefix " << std::endl;

	// write contained groups recursively
	for (gList::const_iterator git = groups.begin();
	     git != groups.end(); ++ git)
	    (*git).second->write(out);
    }
} // ibis::resource::write

/// Write the content to a file.  Default to the log file.
void ibis::resource::write(const char* fn) const {
    if (fn != 0 && *fn != 0) {
	std::ofstream out(fn);
	if (out) {
	    write(out);
	    out.close();
	}
	else if (ibis::gVerbose> -1) {
	    ibis::util::logger lg(ibis::gVerbose - 1);
	    write(lg.buffer());
	}
    }
    else if (ibis::gVerbose> -1) {
	ibis::util::logger lg(ibis::gVerbose - 1);
	write(lg.buffer());
    }
} // ibis::resource::write

/// This implementation uses a local static variable to hold the globally
/// unique resource object.
ibis::resource& ibis::gParameters() {
    static ibis::resource theResource;
    return theResource;
} // ibis::gParameters
