// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008-2010 the Regents of the University of California
#include "jnatural.h"	// ibis::jNatural
#include "tab.h"	// ibis::tabula
#include "bord.h"	// ibis::bord, ibis::bord::bufferList
#include "query.h"	// ibis::query
#include "utilidor.h"	// ibis::util::sortMerge
#include <stdexcept>	// std::exception

ibis::jNatural::jNatural(const ibis::part& partr, const ibis::part& parts,
			 const char* colname, const char* condr,
			 const char* conds)
    : R_(partr), S_(parts), colR_(0), colS_(0), orderR_(0), orderS_(0),
      valR_(0), valS_(0), nrows(-1) {
    if (colname == 0 || *colname == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::jNatural must have a valid string for colname";
	throw "ibis::jNatural must have a valid colname as join columns";
    }

    colR_ = R_.getColumn(colname);
    colS_ = S_.getColumn(colname);
    if (colR_ == 0 || colS_ == 0 || colR_->type() != colS_->type()) {
	if (ibis::gVerbose > 1) {
	    ibis::util::logger lg(1);
	    lg.buffer() << "Warning -- ibis::jNatural ";
	    if (colR_ == 0 || colS_ == 0)
		lg.buffer() << "cannot find column " << colname << " in "
			    << R_.name() << " and " << S_.name();
	    else
		lg.buffer() << "type of " << R_.name() << "." << colname
			    << "(" << ibis::TYPESTRING[colR_->type()]
			    << ") does not match type of " << S_.name()
			    << "." << colname << "("
			    << ibis::TYPESTRING[colS_->type()] << ")";
	}
	throw std::invalid_argument("ibis::jNatural join columns missing "
				    "or having different types");
    }

    int ierr;
    if (condr != 0 && *condr != 0) {
	ibis::query que(ibis::util::userName(), &partr);
	ierr = que.setWhereClause(condr);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural could not parse " << condr
		<< " on partition " << R_.name() << ", ierr = " << ierr;
	    throw "ibis::jNatural failed to parse constraints on R_";
	}
	ierr = que.evaluate();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural could not evaluate "
		<< que.getWhereClause() << " on partition " << R_.name()
		<< ", ierr = " << ierr;
	    throw "ibis::jNatural failed to evaluate constraints on R_";
	}
	maskR_.copy(*que.getHitVector());
    }
    else {
	colR_->getNullMask(maskR_);
    }
    if (conds != 0 && *conds != 0) {
	ibis::query que(ibis::util::userName(), &parts);
	ierr = que.setWhereClause(conds);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural could not parse " << condr
		<< " on partition " << S_.name() << ", ierr = " << ierr;
	    throw "ibis::jNatural failed to parse constraints on S_";
	}
	ierr = que.evaluate();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural could not evaluate "
		<< que.getWhereClause() << " on partition " << S_.name()
		<< ", ierr = " << ierr;
	    throw "ibis::jNatural failed to evaluate constraints on S_";
	}
	maskS_.copy(*que.getHitVector());
    }
    else {
	colR_->getNullMask(maskS_);
    }

    desc_ = R_.name();
    desc_ += " Join ";
    desc_ += S_.name();
    desc_ += " Using(";
    desc_ += colname;
    desc_ += ")";
    if ((condr != 0 && *condr != 0) || (condr != 0 && *condr != 0)) {
	desc_ += " Where ";
	if ((condr != 0 && *condr != 0) && (conds != 0 && *conds != 0)) {
	    desc_ += condr;
	    desc_ += " And ";
	    desc_ += conds;
	}
	else if (condr != 0 && *condr != 0) {
	    desc_ += condr;
	}
	else {
	    desc_ += conds;
	}
    }
    LOGGER(ibis::gVerbose > 2)
	<< "ibis::jNatural[" << desc_ << "] construction complete";
} // ibis::jNatural::jNatural

ibis::jNatural::~jNatural() {
    delete orderR_;
    delete orderS_;
    ibis::bord::freeBuffer(valR_, colR_->type());
    ibis::bord::freeBuffer(valS_, colS_->type());
    LOGGER(ibis::gVerbose > 4)
	<< "ibis::jNatural(" << desc_ << ") freeing...";
} // ibis::jNatural::~jNatural

// Don't do much right now.  May change later.
void ibis::jNatural::estimate(uint64_t& nmin, uint64_t& nmax) {
    nmin = 0;
    nmax = (uint64_t)maskR_.cnt() * maskR_.cnt();
} // ibis::jNatural::estimate

/// Use sort-merge join.  This function sorts the qualified values and
/// counts the number of results.
int64_t ibis::jNatural::evaluate() {
    if (nrows >= 0) return nrows; // already have done this
    if (maskR_.cnt() == 0 || maskS_.cnt() == 0) {
	nrows = 0;
	return nrows;
    }

    std::string mesg;
    mesg = "ibis::jNatural::evaluate(";
    mesg += desc_;
    mesg += ")";
    ibis::util::timer tm(mesg.c_str(), 1);
    // allocate space for ordering arrays
    orderR_ = new array_t<uint32_t>;
    orderS_ = new array_t<uint32_t>;

    // Retrieve and sort the values
    switch (colR_->type()) {
    default:
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::jNatural[" << desc_
	    << "] cann't handle join column of type " << colR_->type();
	return -2;
    case ibis::BYTE: {
	valR_ = colR_->selectBytes(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectBytes("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectBytes(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectBytes("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<char>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<char>*>(valS_),
				      *orderS_);
	break;}
    case ibis::UBYTE: {
	valR_ = colR_->selectUBytes(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectUBytes("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectUBytes(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectUBytes("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge
	    (*static_cast<array_t<unsigned char>*>(valR_),
	     *orderR_,
	     *static_cast<array_t<unsigned char>*>(valS_),
	     *orderS_);
	break;}
    case ibis::SHORT: {
	valR_ = colR_->selectShorts(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectShorts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectShorts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectShorts("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<int16_t>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<int16_t>*>(valS_),
				      *orderS_);
	break;}
    case ibis::USHORT: {
	valR_ = colR_->selectUShorts(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectUShorts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectUShorts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectUShorts("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<uint16_t>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<uint16_t>*>(valS_),
				      *orderS_);
	break;}
    case ibis::INT: {
	valR_ = colR_->selectInts(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectInts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectInts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectInts("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<int32_t>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<int32_t>*>(valS_),
				      *orderS_);
	break;}
    case ibis::UINT: {
	valR_ = colR_->selectUInts(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectUInts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectUInts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectUInts("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<uint32_t>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<uint32_t>*>(valS_),
				      *orderS_);
	break;}
    case ibis::LONG: {
	valR_ = colR_->selectLongs(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectLongs("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectLongs(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectLongs("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<uint64_t>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<uint64_t>*>(valS_),
				      *orderS_);
	break;}
    case ibis::ULONG: {
	valR_ = colR_->selectULongs(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectULongs("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectULongs(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectULongs("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<uint64_t>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<uint64_t>*>(valS_),
				      *orderS_);
	break;}
    case ibis::FLOAT: {
	valR_ = colR_->selectFloats(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectFloats("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectFloats(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectFloats("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<float>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<float>*>(valS_),
				      *orderS_);
	break;}
    case ibis::DOUBLE: {
	valR_ = colR_->selectDoubles(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectDoubles("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectDoubles(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectDoubles("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge(*static_cast<array_t<double>*>(valR_),
				      *orderR_,
				      *static_cast<array_t<double>*>(valS_),
				      *orderS_);
	break;}
    case ibis::TEXT:
    case ibis::CATEGORY: {
	valR_ = colR_->selectStrings(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectStrings("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectStrings(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::jNatural::evaluate(" << desc_
		<< ") call to " << colS_->name() << "->selectStrings("
		<< maskS_.cnt() << ") failed";
	    return -4;
	}
	nrows = ibis::util::sortMerge
	    (*static_cast<std::vector<std::string>* >(valR_),
	     *orderR_,
	     *static_cast<std::vector<std::string>* >(valS_),
	     *orderS_);
        break;}
    }
    return nrows;
} // ibis::jNatural::evaluate

ibis::table*
ibis::jNatural::select(const std::vector<const char*>& colnames) {
    ibis::table *res = 0;
    if (nrows < 0)
	(void) evaluate();
    if (valR_ == 0 || orderR_ == 0 || valS_ == 0 || orderS_ == 0 ||
	orderR_->size() != maskR_.cnt() || orderS_->size() != maskS_.cnt()) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- jNatural::select failed to evaluate the join";
	return res;
    }
    if (colnames.empty() && nrows == 0) {
	std::string nm = ibis::util::shortName(desc_);
	res = new ibis::tabula(nm.c_str(), desc_.c_str(), nrows);
	return res;
    }

    const uint32_t ncols = colnames.size();
    std::string evt;
    evt = "select ";
    evt += colnames[0];
    for (uint32_t j = 1; j < ncols; ++ j) {
	evt += ", ";
	evt += colnames[j];
    }
    evt += " FROM ";
    evt += desc_;
    ibis::util::timer mytimer(evt.c_str());
    const uint32_t rnamelen = strlen(R_.name());
    const uint32_t snamelen = strlen(S_.name());
    std::map<const char*, uint32_t, ibis::lessi> namesToPos;
    std::vector<uint32_t> ipToPos(colnames.size());
    std::vector<const ibis::column*> ircol, iscol;
    // identify the names from the two data partitions
    for (uint32_t j = 0; j < ncols; ++ j) {
	ipToPos[j] = ncols+1;
	const char* cn = colnames[j];
	if (strnicmp(cn, R_.name(), rnamelen) == 0 &&
	    ispunct(cn[rnamelen]) != 0) {
	    cn += rnamelen;
	    while (*cn != 0 && (ispunct(*cn) != 0 || isspace(*cn) != 0)) ++cn;
	    const ibis::column *col = R_.getColumn(cn);
	    if (col != 0) {
		namesToPos[colnames[j]] = j;
		ipToPos[j] = ircol.size();
		ircol.push_back(col);
	    }
	    else {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " can not find column named \""
		    << colnames[j] << "\" in data partition \"" << R_.name()
		    << "\"";
		return res;
	    }
	}
	else if (strnicmp(cn, S_.name(), snamelen) == 0 &&
		 ispunct(cn[snamelen]) != 0) {
	    cn += snamelen;
	    while (*cn != 0 && (ispunct(*cn) != 0 || isspace(*cn) != 0)) ++cn;
	    const ibis::column *col = S_.getColumn(cn);
	    if (col != 0) {
		namesToPos[colnames[j]] = j;
		ipToPos[j] = ncols - iscol.size();
		iscol.push_back(col);
	    }
	    else {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- " << evt << " can not find column named \""
		    << colnames[j] << "\" in data partition \""
		    << S_.name() << "\"";
		return res;
	    }
	}
	else { // not prefixed with a data partition name
	    const ibis::column* col = R_.getColumn(cn);
	    if (col != 0) {
		ipToPos[j] = ircol.size();
		ircol.push_back(col);
		LOGGER(ibis::gVerbose > 3)
		    << evt << " encountered a column name ("
		    << colnames[j] << ") that does not start with a data "
		    "partition name, assume it is for \"" << R_.name() << "\"";
	    }
	    else {
		col = S_.getColumn(cn);
		if (col != 0) {
		    ipToPos[j] = ncols - iscol.size();
		    iscol.push_back(col);
		    LOGGER(ibis::gVerbose > 1)
			<< evt << " encountered a column name (" << colnames[j]
			<< ") that does not start with a data partition name, "
			"assume it is for \"" << S_.name() << "\"";
		}
		else {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- " << evt << " encountered a name ("
			<< colnames[j] << ") that does not start with a data "
			"partition name";
		    return res;
		}
	    }
	}
    } // for (uint32_t j = 0; j < ncols;

    LOGGER(ibis::gVerbose > 3)
	<< evt << " -- found " << ircol.size()
	<< " column" << (ircol.size() > 1 ? "s" : "") << " from "
	<< R_.name() << " and " << iscol.size() << " column"
	<< (iscol.size() > 1 ? "s" : "") << " from " << S_.name();

    // change Pos values for columns in S to have offset ircol.size()
    for (uint32_t j = 0; j < ncols; ++j) {
	if (ipToPos[j] <= ncols && ipToPos[j] >= ircol.size())
	    ipToPos[j] = (ncols - ipToPos[j]) + ircol.size();
    }
    ibis::table::typeList rtypes(ircol.size());
    ibis::bord::bufferList rbuff(ircol.size());
    for (uint32_t j = 0; j < ircol.size(); ++ j) { // initialization
	rtypes[j] = ibis::UNKNOWN_TYPE;
	rbuff[j] = 0;
    }
    ibis::table::typeList stypes(iscol.size());
    ibis::bord::bufferList sbuff(iscol.size());
    for (uint32_t j = 0; j < iscol.size(); ++ j) { // initialization
	stypes[j] = ibis::UNKNOWN_TYPE;
	sbuff[j] = 0;
    }
    bool sane = true;

    // retrieve values from R_
    for (uint32_t j = 0; sane && j < ircol.size(); ++ j) {
	rtypes[j] = ircol[j]->type();
	switch (ircol[j]->type()) {
	case ibis::BYTE:
	    rbuff[j] = ircol[j]->selectBytes(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<signed char>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::UBYTE:
	    rbuff[j] = ircol[j]->selectUBytes(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<unsigned char>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::SHORT:
	    rbuff[j] = ircol[j]->selectShorts(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<int16_t>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::USHORT:
	    rbuff[j] = ircol[j]->selectUShorts(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<uint16_t>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::INT:
	    rbuff[j] = ircol[j]->selectInts(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<int32_t>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::UINT:
	    rbuff[j] = ircol[j]->selectUInts(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<uint32_t>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::LONG:
	    rbuff[j] = ircol[j]->selectLongs(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<int64_t>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::ULONG:
	    rbuff[j] = ircol[j]->selectULongs(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<uint64_t>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::FLOAT:
	    rbuff[j] = ircol[j]->selectFloats(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<float>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::DOUBLE:
	    rbuff[j] = ircol[j]->selectDoubles(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<double>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    rbuff[j] = ircol[j]->selectStrings(maskR_);
	    if (rbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<std::vector<std::string>*>(rbuff[j]),
		     *orderR_);
	    else
		sane = false;
	    break;
	default:
	    sane = false;
	    rbuff[j] = 0;
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- jNatural::select does not support column "
		"type " << ibis::TYPESTRING[(int)ircol[j]->type()]
		<< " (name = " << R_.name() << "." << ircol[j]->name() << ")";
	    break;
	}
    }
    if (! sane) {
	ibis::bord::freeBuffers(rbuff, rtypes);
	return res;
    }

    // retrieve values from S_
    for (uint32_t j = 0; sane && j < iscol.size(); ++ j) {
	stypes[j] = iscol[j]->type();
	switch (iscol[j]->type()) {
	case ibis::BYTE:
	    sbuff[j] = iscol[j]->selectBytes(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<signed char>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::UBYTE:
	    sbuff[j] = iscol[j]->selectUBytes(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<unsigned char>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::SHORT:
	    sbuff[j] = iscol[j]->selectShorts(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<int16_t>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::USHORT:
	    sbuff[j] = iscol[j]->selectUShorts(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<uint16_t>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::INT:
	    sbuff[j] = iscol[j]->selectInts(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<int32_t>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::UINT:
	    sbuff[j] = iscol[j]->selectUInts(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<uint32_t>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::LONG:
	    sbuff[j] = iscol[j]->selectLongs(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<int64_t>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::ULONG:
	    sbuff[j] = iscol[j]->selectULongs(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<uint64_t>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::FLOAT:
	    sbuff[j] = iscol[j]->selectFloats(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<float>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::DOUBLE:
	    sbuff[j] = iscol[j]->selectDoubles(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<array_t<double>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    sbuff[j] = iscol[j]->selectStrings(maskS_);
	    if (sbuff[j] != 0)
		ibis::util::reorder
		    (*static_cast<std::vector<std::string>*>(sbuff[j]),
		     *orderS_);
	    else
		sane = false;
	    break;
	default:
	    sane = false;
	    sbuff[j] = 0;
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- jNatural::select does not support column "
		"type " << ibis::TYPESTRING[(int)iscol[j]->type()]
		<< " (name = " << S_.name() << "." << iscol[j]->name() << ")";
	    break;
	}
    }
    if (! sane) {
	ibis::bord::freeBuffers(rbuff, rtypes);
	ibis::bord::freeBuffers(sbuff, stypes);
	return res;
    }

    /// fill the in-memory buffer
    switch (colR_->type()) {
    case ibis::BYTE:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<signed char>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<signed char>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::UBYTE:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<unsigned char>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<unsigned char>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::SHORT:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<int16_t>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<int16_t>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::USHORT:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<uint16_t>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<uint16_t>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::INT:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<int32_t>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<int32_t>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::UINT:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<uint32_t>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<uint32_t>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::LONG:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<int64_t>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<int64_t>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::ULONG:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<uint64_t>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<uint64_t>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::FLOAT:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<float>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<float>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::DOUBLE:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<array_t<double>*>(valR_), rtypes, rbuff,
	     *static_cast<array_t<double>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	res = ibis::join::fillEquiJoinTable
	    (nrows, evt,
	     *static_cast<std::vector<std::string>*>(valR_), rtypes, rbuff,
	     *static_cast<std::vector<std::string>*>(valS_), stypes, sbuff,
	     colnames, ipToPos);
	break;
    default:
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- " << evt << " can not handle join column of type "
	    << ibis::TYPESTRING[(int)colR_->type()];
    }
    return res;
} // ibis::jNatural::select

ibis::join* ibis::join::create(const ibis::part& partr, const ibis::part& parts,
			       const char* colname, const char* condr,
			       const char* conds) {
    return new ibis::jNatural(partr, parts, colname, condr, conds);
} // ibis::join::create

/// Generate a table representing an equi-join in memory.  The input to
/// this function are values to go into the resulting table.  It only needs
/// to match the rows and fill the output table.
template <typename T>
ibis::table*
ibis::join::fillEquiJoinTable(size_t nrows,
			      const std::string &desc,
			      const ibis::array_t<T>& rjcol,
			      const ibis::table::typeList& rtypes,
			      const std::vector<void*>& rbuff,
			      const ibis::array_t<T>& sjcol,
			      const ibis::table::typeList& stypes,
			      const ibis::bord::bufferList& sbuff,
			      const ibis::table::stringList& tcname,
			      const std::vector<uint32_t>& tcnpos) {
    if (rjcol.empty() || sjcol.empty() ||
	(nrows > rjcol.size() * sjcol.size()) ||
	(nrows < rjcol.size() && nrows < sjcol.size()) ||
	((rtypes.empty() || rbuff.empty() || rtypes.size() != rbuff.size()) &&
	 (stypes.empty() || sbuff.empty() || stypes.size() != sbuff.size())) ||
	tcname.size() != rtypes.size() + stypes.size() ||
	tcnpos.size() != tcname.size()) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- join::fillEquiJoinTable can not proceed due "
	    "to invalid arguments";
	return 0;
    }

    ibis::bord::bufferList tbuff(tcname.size());
    ibis::table::typeList ttypes(tcname.size());
    try {
	bool badpos = false;
	// allocate enough space for the 
	for (size_t j = 0; j < tcname.size(); ++ j) {
	    if (tcnpos[j] < rtypes.size()) {
		ttypes[j] = rtypes[tcnpos[j]];
		tbuff[j] = ibis::bord::allocateBuffer(rtypes[tcnpos[j]], nrows);
	    }
	    else if (tcnpos[j] < rtypes.size()+stypes.size()) {
		ttypes[j] = stypes[tcnpos[j]-rtypes.size()];
		tbuff[j] = ibis::bord::allocateBuffer
		    (stypes[tcnpos[j]-rtypes.size()], nrows);
	    }
	    else { // tcnpos is out of valid range
		ttypes[j] = ibis::UNKNOWN_TYPE;
		tbuff[j] = 0;
		badpos = true;
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- join::fillEquiJoinTable detects an "
		    "invalid tcnpos[" << j << "] = " << tcnpos[j]
		    << ", should be less than " << rtypes.size()+stypes.size();
	    }
	}
	if (badpos) {
	    ibis::bord::freeBuffers(tbuff, ttypes);
	    return 0;
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- join::fillEquiJoinTable failed to allocate "
	    "sufficient memory for " << nrows << " row" << (nrows>1?"s":"")
	    << " and " << rtypes.size()+stypes.size()
	    << " column" << (rtypes.size()+stypes.size()>1?"s":"");
	ibis::bord::freeBuffers(tbuff, ttypes);
	return 0;
    }

    size_t tind = 0; // row index into the resulting table
    for (size_t rind = 0, sind = 0;
	 rind < rjcol.size() && sind < sjcol.size(); ) {
	while (rind < rjcol.size() && rjcol[rind] < sjcol[sind]) ++ rind;
	while (sind < sjcol.size() && sjcol[sind] < rjcol[rind]) ++ sind;
	if (rind < rjcol.size() && sind < sjcol.size() &&
	    rjcol[rind] == sjcol[sind]) { // found matches
	    size_t rind0 = rind;
	    size_t rind1 = rind+1;
	    while (rind1 < rjcol.size() && rjcol[rind1] == sjcol[sind])
		++ rind1;
	    size_t sind0 = sind;
	    size_t sind1 = sind+1;
	    while (sind1 < sjcol.size() && sjcol[sind1] == rjcol[rind])
		++ sind1;
	    for (rind = rind0; rind < rind1; ++ rind) {
		for (sind = sind0; sind < sind1; ++ sind) {
		    for (size_t j = 0; j < tcnpos.size(); ++ j) {
			if (tcnpos[j] < rbuff.size()) {
			    ibis::bord::copyValue(rtypes[tcnpos[j]],
						  tbuff[j], tind,
						  rbuff[tcnpos[j]], rind);
			}
			else {
			    ibis::bord::copyValue
				(stypes[tcnpos[j-rtypes.size()]],
				 tbuff[j], tind,
				 sbuff[tcnpos[j-rtypes.size()]], sind);
			}
		    } // j
		    ++ tind;
		} // sind
	    } // rind
	}
    } // for (size_t rind = 0, sind = 0; ...
    if (tind != nrows) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- join::fillEquiJoinTable expected to produce "
	    << nrows << " row" << (nrows>1?"s":"") << ", but produced "
	    << tind << " instead";
	ibis::bord::freeBuffers(tbuff, ttypes);
	return 0;
    }

    std::string tn = ibis::util::shortName(desc.c_str());
    return new ibis::bord(tn.c_str(), desc.c_str(), nrows,
			  tbuff, ttypes, tcname);
} // ibis::join::fillEquiJoinTable

ibis::table*
ibis::join::fillEquiJoinTable(size_t nrows,
			      const std::string &desc,
			      const std::vector<std::string>& rjcol,
			      const ibis::table::typeList& rtypes,
			      const std::vector<void*>& rbuff,
			      const std::vector<std::string>& sjcol,
			      const ibis::table::typeList& stypes,
			      const ibis::bord::bufferList& sbuff,
			      const ibis::table::stringList& tcname,
			      const std::vector<uint32_t>& tcnpos) {
    if (rjcol.empty() || sjcol.empty() ||
	(nrows > rjcol.size() * sjcol.size()) ||
	(nrows < rjcol.size() && nrows < sjcol.size()) ||
	((rtypes.empty() || rbuff.empty() || rtypes.size() != rbuff.size()) &&
	 (stypes.empty() || sbuff.empty() || stypes.size() != sbuff.size())) ||
	tcname.size() != rtypes.size() + stypes.size() ||
	tcnpos.size() != tcname.size()) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- join::fillEquiJoinTable can not proceed due "
	    "to invalid arguments";
	return 0;
    }

    ibis::bord::bufferList tbuff(tcname.size());
    ibis::table::typeList ttypes(tcname.size());
    try {
	bool badpos = false;
	// allocate enough space for the 
	for (size_t j = 0; j < tcname.size(); ++ j) {
	    if (tcnpos[j] < rtypes.size()) {
		ttypes[j] = rtypes[tcnpos[j]];
		tbuff[j] = ibis::bord::allocateBuffer(rtypes[tcnpos[j]], nrows);
	    }
	    else if (tcnpos[j] < rtypes.size()+stypes.size()) {
		ttypes[j] = stypes[tcnpos[j]-rtypes.size()];
		tbuff[j] = ibis::bord::allocateBuffer
		    (stypes[tcnpos[j]-rtypes.size()], nrows);
	    }
	    else { // tcnpos is out of valid range
		ttypes[j] = ibis::UNKNOWN_TYPE;
		tbuff[j] = 0;
		badpos = true;
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- join::fillEquiJoinTable detects an "
		    "invalid tcnpos[" << j << "] = " << tcnpos[j]
		    << ", should be less than " << rtypes.size()+stypes.size();
	    }
	}
	if (badpos) {
	    ibis::bord::freeBuffers(tbuff, ttypes);
	    return 0;
	}
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- join::fillEquiJoinTable failed to allocate "
	    "sufficient memory for " << nrows << " row" << (nrows>1?"s":"")
	    << " and " << rtypes.size()+stypes.size()
	    << " column" << (rtypes.size()+stypes.size()>1?"s":"");
	ibis::bord::freeBuffers(tbuff, ttypes);
	return 0;
    }

    size_t tind = 0; // row index into the resulting table
    for (size_t rind = 0, sind = 0;
	 rind < rjcol.size() && sind < sjcol.size(); ) {
	while (rind < rjcol.size() && rjcol[rind] < sjcol[sind]) ++ rind;
	while (sind < sjcol.size() && sjcol[sind] < rjcol[rind]) ++ sind;
	if (rind < rjcol.size() && sind < sjcol.size() &&
	    rjcol[rind] == sjcol[sind]) {
	    size_t rind0 = rind;
	    size_t rind1 = rind+1;
	    while (rind1 < rjcol.size() && rjcol[rind1] == sjcol[sind])
		++ rind1;
	    size_t sind0 = sind;
	    size_t sind1 = sind+1;
	    while (sind1 < sjcol.size() && sjcol[sind1] == rjcol[rind])
		++ sind1;
	    for (rind = rind0; rind < rind1; ++ rind) {
		for (sind = sind0; sind < sind1; ++ sind) {
		    for (size_t j = 0; j < tcnpos.size(); ++ j) {
			if (tcnpos[j] < rbuff.size()) {
			    ibis::bord::copyValue(rtypes[tcnpos[j]],
						  tbuff[j], tind,
						  rbuff[tcnpos[j]], rind);
			}
			else {
			    ibis::bord::copyValue
				(stypes[tcnpos[j-rtypes.size()]],
				 tbuff[j], tind,
				 sbuff[tcnpos[j-rtypes.size()]], sind);
			}
		    } // j
		    ++ tind;
		} // sind
	    } // rind
	}
    } // for (size_t rind = 0, sind = 0; ...
    if (tind != nrows) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- join::fillEquiJoinTable expected to produce "
	    << nrows << " row" << (nrows>1?"s":"") << ", but produced "
	    << tind << " instead";
	ibis::bord::freeBuffers(tbuff, ttypes);
	return 0;
    }

    std::string tn = ibis::util::shortName(desc.c_str());
    return new ibis::bord(tn.c_str(), desc.c_str(), nrows,
			  tbuff, ttypes, tcname);
} // ibis::join::fillEquiJoinTable
