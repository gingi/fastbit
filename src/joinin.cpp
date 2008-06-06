// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2008 the Regents of the University of California
#include "joinin.h"	// ibis::joinIN
#include "query.h"	// ibis::query
#include "utilidor.h"	// ibis::util::sortMerge

ibis::joinIN::joinIN(const ibis::part& partr, const ibis::part& parts,
		     const char* colname, const char* condr, const char* conds)
    : R_(partr), S_(parts), colR_(0), colS_(0), orderR_(0), orderS_(0),
      valR_(0), valS_(0), nrows(-1) {
    if (colname == 0 || *colname == 0) {
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::joinIN must have a valid string for colname";
	throw "ibis::joinIN must have a valid colname as join columns";
    }

    colR_ = R_.getColumn(colname);
    colS_ = S_.getColumn(colname);
    if (colR_ == 0 || colS_ == 0 || colR_->type() != colS_->type()) {
	if (ibis::gVerbose > 1) {
	    ibis::util::logger lg(1);
	    lg.buffer() << "Warning -- ibis::joinIN ";
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
	throw "ibis::joinIN join columns donot exist or having different types";
    }

    int ierr;
    if (condr != 0 && *condr != 0) {
	ibis::query que(ibis::util::userName(), &partr);
	ierr = que.setWhereClause(condr);
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN could not parse " << condr
		<< " on partition " << R_.name() << ", ierr = " << ierr;
	    throw "ibis::joinIN failed to parse constraints on R_";
	}
	ierr = que.evaluate();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN could not evaluate "
		<< que.getWhereClause() << " on partition " << R_.name()
		<< ", ierr = " << ierr;
	    throw "ibis::joinIN failed to evaluate constraints on R_";
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
		<< "Warning -- ibis::joinIN could not parse " << condr
		<< " on partition " << S_.name() << ", ierr = " << ierr;
	    throw "ibis::joinIN failed to parse constraints on S_";
	}
	ierr = que.evaluate();
	if (ierr < 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN could not evaluate "
		<< que.getWhereClause() << " on partition " << S_.name()
		<< ", ierr = " << ierr;
	    throw "ibis::joinIN failed to evaluate constraints on S_";
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
	<< "ibis::joinIN[" << desc_ << "] construction complete";
} // ibis::joinIN::joinIN

ibis::joinIN::~joinIN() {
    delete orderR_;
    delete orderS_;
    ibis::joinIN::freeBuffer(valR_, colR_->type());
    ibis::joinIN::freeBuffer(valS_, colS_->type());
    LOGGER(ibis::gVerbose > 4)
	<< "ibis::joinIN(" << desc_ << ") freeing...";
} // ibis::joinIN::~joinIN

// Don't do much right now.  May change later.
void ibis::joinIN::estimate(uint64_t& nmin, uint64_t& nmax) {
    nmin = 0;
    nmax = (uint64_t)maskR_.cnt() * maskR_.cnt();
} // ibis::joinIN::estimate

// Use sort-merge join.  This function sorts the qualified values and
// counts the number of results.
int64_t ibis::joinIN::evaluate() {
    if (nrows >= 0) return nrows; // already have done this
    if (maskR_.cnt() == 0 || maskS_.cnt() == 0) {
	nrows = 0;
	return nrows;
    }

    std::string mesg;
    mesg = "ibis::joinIN::evaluate(";
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
	    << "Warning -- ibis::joinIN[" << desc_
	    << "] cann't handle join column of type " << colR_->type();
	return -2;
    case ibis::BYTE: {
	valR_ = colR_->selectBytes(maskR_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectBytes("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectBytes(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectUBytes("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectUBytes(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectShorts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectShorts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectUShorts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectUShorts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectInts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectInts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectUInts("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectUInts(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectLongs("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectLongs(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectULongs("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectULongs(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectFloats("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectFloats(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectDoubles("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectDoubles(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
		<< ") call to " << colR_->name() << "->selectStrings("
		<< maskR_.cnt() << ") failed";
	    return -3;
	}
	valS_ = colS_->selectStrings(maskS_);
	if (valR_ == 0) {
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::evaluate(" << desc_
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
} // ibis::joinIN::evaluate

ibis::join::result*
ibis::joinIN::select(const std::vector<const char*>& colnames) {
    ibis::join::result *res = 0;
    if (colnames.empty()) return res;
    if (nrows < 0)
	(void) evaluate();
    if (valR_ != 0 && orderR_ != 0 && valS_ != 0 && orderS_ != 0 &&
	orderR_->size() == maskR_.cnt() && orderS_->size() == maskS_.cnt()) {
	try {
	    res = new ibis::joinIN::result(*this, colnames);
	}
	catch (...) {
	    res = 0;
	}
    }
    return res;
} // ibis::joinIN::select

void ibis::joinIN::freeBuffer(void *buffer, ibis::TYPE_T type) {
    switch (type) {
    default:
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::joinIN::freeBuffer(" << buffer << ", "
	    << (int) type << ") unable to handle data type "
	    << ibis::TYPESTRING[(int)type];
	break;
    case ibis::BYTE:
	delete static_cast<array_t<char>*>(buffer);
	break;
    case ibis::UBYTE:
	delete static_cast<array_t<unsigned char>*>(buffer);
	break;
    case ibis::SHORT:
	delete static_cast<array_t<int16_t>*>(buffer);
	break;
    case ibis::USHORT:
	delete static_cast<array_t<uint16_t>*>(buffer);
	break;
    case ibis::INT:
	delete static_cast<array_t<int32_t>*>(buffer);
	break;
    case ibis::UINT:
	delete static_cast<array_t<uint32_t>*>(buffer);
	break;
    case ibis::LONG:
	delete static_cast<array_t<int64_t>*>(buffer);
	break;
    case ibis::ULONG:
	delete static_cast<array_t<uint64_t>*>(buffer);
	break;
    case ibis::FLOAT:
	delete static_cast<array_t<float>*>(buffer);
	break;
    case ibis::DOUBLE:
	delete static_cast<array_t<double>*>(buffer);
	break;
    case ibis::TEXT:
    case ibis::CATEGORY:
	delete static_cast<std::vector<std::string>*>(buffer);
        break;
    }
} // ibis::joinIN::freeBuffer

ibis::joinIN::result::result(const ibis::joinIN& jin,
			     const std::vector<const char*>& colnames)
    : jin_(jin), endR_(jin.maskR_.cnt()), endS_(jin.maskS_.cnt()),
      currR_(0), currS_(0), blockR_(0), blockS_(0), startS_(0) {
    const size_t rnamelen = strlen(jin.R_.name());
    const size_t snamelen = strlen(jin.S_.name());
    ipToPos.resize(colnames.size());
    for (size_t j = 0; j < colnames.size(); ++ j) {
	ipToPos[j] = colnames.size()+1;
	const char* cn = colnames[j];
	if (strnicmp(cn, jin.R_.name(), rnamelen) == 0) {
	    cn += rnamelen;
	    while (*cn != 0 && (ispunct(*cn) != 0 || isspace(*cn))) ++cn;
	    const ibis::column *col = jin.R_.getColumn(cn);
	    if (col != 0) {
		bool add = true;
		for (size_t i = 0; i < colR_.size(); ++ i) {
		    if (col == colR_[i]) {
			add = false;
			break;
		    }
		}
		if (add) {
		    namesToPos[colnames[j]] = j;
		    ipToPos[j] = colR_.size();
		    colR_.push_back(col);
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- ibis::joinIN::result can not find column "
		    "named \"" << colnames[j] << "\" in data partition \""
		    << jin.R_.name() << "\", skip the name";
	    }
	}
	else if (strnicmp(cn, jin.S_.name(), snamelen) == 0) {
	    cn += snamelen;
	    while (*cn != 0 && (ispunct(*cn) != 0 || isspace(*cn))) ++cn;
	    const ibis::column *col = jin.S_.getColumn(cn);
	    if (col != 0) {
		bool add = true;
		for (size_t i = 0; i < colS_.size(); ++ i) {
		    if (col == colS_[i]) {
			add = false;
			break;
		    }
		}
		if (add) {
		    namesToPos[colnames[j]] = j;
		    ipToPos[j] = colnames.size() - colS_.size();
		    colS_.push_back(col);
		}
	    }
	    else {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- ibis::joinIN::result can not find column "
		    "named \"" << colnames[j] << "\" in data partition \""
		    << jin.S_.name() << "\", skip the name";
	    }
	}
	else {
	    const ibis::column* col = jin.R_.getColumn(cn);
	    if (col != 0) {
		bool savetor = true;
		for (size_t i = 0; i < colR_.size(); ++ i) {
		    if (col == colR_[i]) {
			savetor = false;
			break;
		    }
		}
		if (savetor) {
		    ipToPos[j] = colR_.size();
		    colR_.push_back(col);
		    LOGGER(ibis::gVerbose > 1)
			<< "ibis::joinIN::result encountered a column name ("
			<< colnames[j] << ") that does not start with a data "
			"partition name, assume it is for \"" << jin.R_.name()
			<< "\"";
		}
		else {
		    col = 0;
		}
	    }
	    if (col == 0) {
		col = jin.S_.getColumn(cn);
		if (col != 0) {
		    bool add = true;
		    for (size_t i = 0; i < colS_.size(); ++ i) {
			if (col == colS_[i]) {
			    add = false;
			    break;
			}
		    }
		    if (add) {
			ipToPos[j] = colnames.size() - colS_.size();
			colS_.push_back(col);
		    }
		    LOGGER(ibis::gVerbose > 1)
			<< "ibis::joinIN::result encountered a column name ("
			<< colnames[j] << ") that does not start with a data "
			"partition name, assume it is for \"" << jin.S_.name()
			<< "\"";
		}
		else {
		    LOGGER(ibis::gVerbose > 1)
			<< "ibis::joinIN::result encountered a column name ("
			<< colnames[j] << ") that does not start with a data "
			"partition name, discard the name";
		}
	    }
	}
    } // for (size_t j = 0; j < colnames.size();

    if (colR_.empty() || colS_.empty()) {
	LOGGER(ibis::gVerbose >= 0)
	    << "Warning -- ibis::joinIN::result found " << colR_.size()
	    << " column" << (colR_.size() > 1 ? "s" : "") << " from "
	    << jin.R_.name() << " and " << colS_.size() << " column"
	    << (colS_.size() > 1 ? "s" : "") << " from " << jin.S_.name()
	    << ", but need both lists of columns to be not empty";
	throw "ibis::joinIN::result can not work with empty column lists";
    }

    // change Pos values for columns in S to have offset colR_.size()
    for (size_t j = 0; j < colnames.size(); ++j) {
	if (ipToPos[j] <= colnames.size() && ipToPos[j] >= colR_.size())
	    ipToPos[j] = (colnames.size() - ipToPos[j]) + colR_.size();
    }

    // retrieve values from R_
    valR_.resize(colR_.size());
    typeR_.resize(colR_.size());
    for (size_t j = 0; j < colR_.size(); ++ j) {
	typeR_[j] = colR_[j]->type();
	switch (typeR_[j]) {
	default:
	    valR_[j] = 0;
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::result does not support column "
		"type " << ibis::TYPESTRING[(int)typeR_[j]] << " (name = "
		<< jin.R_.name() << "." << colR_[j]->name() << ")";
	case ibis::BYTE:
	    valR_[j] = colR_[j]->selectBytes(jin.maskR_);
	    break;
	case ibis::UBYTE:
	    valR_[j] = colR_[j]->selectUBytes(jin.maskR_);
	    break;
	case ibis::SHORT:
	    valR_[j] = colR_[j]->selectShorts(jin.maskR_);
	    break;
	case ibis::USHORT:
	    valR_[j] = colR_[j]->selectUShorts(jin.maskR_);
	    break;
	case ibis::INT:
	    valR_[j] = colR_[j]->selectInts(jin.maskR_);
	    break;
	case ibis::UINT:
	    valR_[j] = colR_[j]->selectUInts(jin.maskR_);
	    break;
	case ibis::LONG:
	    valR_[j] = colR_[j]->selectLongs(jin.maskR_);
	    break;
	case ibis::ULONG:
	    valR_[j] = colR_[j]->selectULongs(jin.maskR_);
	    break;
	case ibis::FLOAT:
	    valR_[j] = colR_[j]->selectFloats(jin.maskR_);
	    break;
	case ibis::DOUBLE:
	    valR_[j] = colR_[j]->selectDoubles(jin.maskR_);
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    valR_[j] = colR_[j]->selectStrings(jin.maskR_);
	    break;
	}
    }

    // retrieve values from S_
    valS_.resize(colS_.size());
    typeS_.resize(colS_.size());
    for (size_t j = 0; j < colS_.size(); ++ j) {
	typeS_[j] = colS_[j]->type();
	switch (typeS_[j]) {
	default:
	    valS_[j] = 0;
	    LOGGER(ibis::gVerbose > 1)
		<< "Warning -- ibis::joinIN::result does not support column "
		"type " << ibis::TYPESTRING[(int)typeS_[j]] << " (name = "
		<< jin.S_.name() << "." << colS_[j]->name() << ")";
	case ibis::BYTE:
	    valS_[j] = colS_[j]->selectBytes(jin.maskS_);
	    break;
	case ibis::UBYTE:
	    valS_[j] = colS_[j]->selectUBytes(jin.maskS_);
	    break;
	case ibis::SHORT:
	    valS_[j] = colS_[j]->selectShorts(jin.maskS_);
	    break;
	case ibis::USHORT:
	    valS_[j] = colS_[j]->selectUShorts(jin.maskS_);
	    break;
	case ibis::INT:
	    valS_[j] = colS_[j]->selectInts(jin.maskS_);
	    break;
	case ibis::UINT:
	    valS_[j] = colS_[j]->selectUInts(jin.maskS_);
	    break;
	case ibis::LONG:
	    valS_[j] = colS_[j]->selectLongs(jin.maskS_);
	    break;
	case ibis::ULONG:
	    valS_[j] = colS_[j]->selectULongs(jin.maskS_);
	    break;
	case ibis::FLOAT:
	    valS_[j] = colS_[j]->selectFloats(jin.maskS_);
	    break;
	case ibis::DOUBLE:
	    valS_[j] = colS_[j]->selectDoubles(jin.maskS_);
	    break;
	case ibis::TEXT:
	case ibis::CATEGORY:
	    valS_[j] = colS_[j]->selectStrings(jin.maskS_);
	    break;
	}
    }

    LOGGER(ibis::gVerbose > 1)
	<< "ibis::joinIN::result construction completed successfully";
} // ibis::joinIN::result::result

ibis::joinIN::result::~result() {
    for (size_t j = 0; j < valR_.size(); ++ j) {
	ibis::joinIN::freeBuffer(valR_[j], typeR_[j]);
    }
    for (size_t j = 0; j < valS_.size(); ++ j) {
	ibis::joinIN::freeBuffer(valS_[j], typeS_[j]);
    }
    LOGGER(ibis::gVerbose > 5)
	<< "ibis::joinIN::result(0x" << this << ") freeing ...";
} // ibis::joinIN::result::~result

void ibis::joinIN::result::describe(std::ostream& out) const {
    const size_t ncols = colR_.size() + colS_.size();
    out << "Result of selecting " << ncols << " column"
	<< (ncols>1?"s":"") << " from " << jin_.desc_ << " ("
	<< jin_.nrows << " row" << (jin_.nrows>1?"s":"") << ")\n";
    for (size_t j = 0; j < ipToPos.size(); ++ j) {
	uint32_t i = ipToPos[j];
	if (i < colR_.size()) {
	    out << jin_.R_.name() << "." << colR_[i]->name() << "\t"
		<< ibis::TYPESTRING[(int)typeR_[i]] << "\n";
	}
	else {
	    i -= colR_.size();
	    if (i < colS_.size())
		out << jin_.S_.name() << "." << colS_[i]->name() << "\t"
		    << ibis::TYPESTRING[(int)typeS_[i]] << "\n";
	}
    }
} // ibis::joinIN::result::describe

std::vector<std::string> ibis::joinIN::result::columnNames() const {
    std::vector<std::string> res;
    for (size_t j = 0; j < colR_.size(); ++ j) {
	std::string name = jin_.R_.name();
	name += '.';
	name += colR_[j]->name();
	res.push_back(name);
    }
    for (size_t j = 0; j < colS_.size(); ++ j) {
	std::string name = jin_.S_.name();
	name += '.';
	name += colS_[j]->name();
	res.push_back(name);
    }
    return res;
} // ibis::joinIN::result::columnNames

ibis::table::typeList ibis::joinIN::result::columnTypes() const {
    ibis::table::typeList res;
    for (size_t j = 0; j < typeR_.size(); ++ j)
	res.push_back(typeR_[j]);
    for (size_t j = 0; j < typeS_.size(); ++ j)
	res.push_back(typeS_[j]);
    return res;
} // ibis::joinIN::result::columnTypes

int ibis::joinIN::result::fetch() {
    int ready = -1;
    if (jin_.colR_ == 0 || jin_.colS_ == 0 || jin_.valR_ == 0 ||
	jin_.valS_ == 0 || jin_.orderR_ == 0 || jin_.orderS_ == 0) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- ibis::joinIN::result points to an invalid "
	    "ibis::joinIN object";
	return ready;
    }

    switch (jin_.colR_->type()) {
    default:
	LOGGER(ibis::gVerbose > 1)
	    << "Warning -- ibis::joinIN::result::fetch unable to work with "
	    "join column of type "
	    << ibis::TYPESTRING[(int)(jin_.colR_->type())];
	break;
    case ibis::BYTE: {
	const array_t<char>& jcolR = *static_cast<array_t<char>*>(jin_.valR_);
	const array_t<char>& jcolS = *static_cast<array_t<char>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::UBYTE: {
	const array_t<unsigned char>& jcolR =
	    *static_cast<array_t<unsigned char>*>(jin_.valR_);
	const array_t<unsigned char>& jcolS =
	    *static_cast<array_t<unsigned char>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::SHORT: {
	const array_t<int16_t>& jcolR = 
	    *static_cast<array_t<int16_t>*>(jin_.valR_);
	const array_t<int16_t>& jcolS =
	    *static_cast<array_t<int16_t>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::USHORT: {
	const array_t<uint16_t>& jcolR =
	    *static_cast<array_t<uint16_t>*>(jin_.valR_);
	const array_t<uint16_t>& jcolS =
	    *static_cast<array_t<uint16_t>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::INT: {
	const array_t<int32_t>& jcolR =
	    *static_cast<array_t<int32_t>*>(jin_.valR_);
	const array_t<int32_t>& jcolS =
	    *static_cast<array_t<int32_t>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::UINT: {
	const array_t<uint32_t>& jcolR =
	    *static_cast<array_t<uint32_t>*>(jin_.valR_);
	const array_t<uint32_t>& jcolS =
	    *static_cast<array_t<uint32_t>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::LONG: {
	const array_t<int64_t>& jcolR =
	    *static_cast<array_t<int64_t>*>(jin_.valR_);
	const array_t<int64_t>& jcolS =
	    *static_cast<array_t<int64_t>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::ULONG: {
	const array_t<uint64_t>& jcolR =
	    *static_cast<array_t<uint64_t>*>(jin_.valR_);
	const array_t<uint64_t>& jcolS =
	    *static_cast<array_t<uint64_t>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::FLOAT: {
	const array_t<float>& jcolR =
	    *static_cast<array_t<float>*>(jin_.valR_);
	const array_t<float>& jcolS =
	    *static_cast<array_t<float>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::DOUBLE: {
	const array_t<double>& jcolR =
	    *static_cast<array_t<double>*>(jin_.valR_);
	const array_t<double>& jcolS =
	    *static_cast<array_t<double>*>(jin_.valS_);
	ready = nextMatch(jcolR, jcolS);
	break;}
    case ibis::TEXT:
    case ibis::CATEGORY: {
	const std::vector<std::string>& jcolR =
	    *static_cast<std::vector<std::string>*>(jin_.valR_);
    const std::vector<std::string>& jcolS =
	*static_cast<std::vector<std::string>*>(jin_.valS_);
	ready = stringMatch(jcolR, jcolS);
	break;}
    }
    return ready;
} // ibis::joinIN::result::fetch

template <typename T> int
ibis::joinIN::result::nextMatch(const array_t<T>& col1,
				const array_t<T>& col2) {
    if (currS_ < blockS_ && currR_ < blockR_) {
	if (1+currS_ < blockS_) {
	    ++ currS_;
	    return 0;
	}
	else if (1+currR_ < blockR_) {
	    ++ currR_;
	    currS_ = startS_;
	    return 0;
	}
	else {
	    currR_ = blockR_;
	    currS_ = blockS_;
	}
    }

    while (currR_ < endR_ && currS_ < endS_ && col1[currR_] != col2[currS_]) {
	while (currR_ < endR_ && col1[currR_] < col2[currS_])
	    ++ currR_;
	if (currR_ < endR_) {
	    while (currS_ < endS_ && col1[currR_] > col2[currS_])
		++ currS_;
	}
    }
    if (currR_ < endR_ && currS_ < endS_ && col1[currR_] == col2[currS_]) {
	if (blockR_ <= currR_) {
	    for (blockR_ = currR_+1;
		 blockR_ < endR_ && col1[blockR_] == col1[currR_];
		 ++ blockR_);
	}
	if (blockS_ <= currS_) {
	    startS_ = currS_;
	    for (blockS_ = currS_+1;
		 blockS_ < endS_ && col1[blockS_] == col1[currS_];
		 ++ blockS_);
	}
    }
    return (int)(currR_ < blockR_ && currS_ < blockS_) - 1;
} // ibis::joinIN::result::nextMatch

int ibis::joinIN::result::stringMatch(const std::vector<std::string>& col1,
				      const std::vector<std::string>& col2) {
    if (currS_ < blockS_ && currR_ < blockR_) {
	if (1+currS_ < blockS_) {
	    ++ currS_;
	    return 0;
	}
	else if (1+currR_ < blockR_) {
	    ++ currR_;
	    currS_ = startS_;
	    return 0;
	}
	else {
	    currR_ = blockR_;
	    currS_ = blockS_;
	}
    }

    while (currR_ < endR_ && currS_ < endS_ &&
	   col1[currR_].compare(col2[currS_]) != 0) {
	while (currR_ < endR_ && col1[currR_].compare(col2[currS_]) < 0)
	    ++ currR_;
	if (currR_ < endR_) {
	    while (currS_ < endS_ && col1[currR_].compare(col2[currS_]) > 0)
		++ currS_;
	}
    }
    if (currR_ < endR_ && currS_ < endS_ &&
	col1[currR_].compare(col2[currS_]) == 0) {
	if (blockR_ <= currR_) {
	    for (blockR_ = currR_+1;
		 blockR_ < endR_ && col1[blockR_].compare(col1[currR_]) == 0;
		 ++ blockR_);
	}
	if (blockS_ <= currS_) {
	    startS_ = currS_;
	    for (blockS_ = currS_+1;
		 blockS_ < endS_ && col1[blockS_].compare(col1[currS_]) == 0;
		 ++ blockS_);
	}
    }
    return (int)(currR_ < blockR_ && currS_ < blockS_) - 1;
} // ibis::joinIN::result::stringMatch

int ibis::joinIN::result::dump(std::ostream& out, const char* del) const {
    if (currR_ >= blockR_ || currS_ >= blockS_) return -1;
    if (del != 0 && *del != 0) {
	for (size_t j = 0; j < ipToPos.size(); ++ j) {
	    uint32_t i = ipToPos[j];
	    if (i < colR_.size()) {
		dumpR(out, i);
		if (j+1 < ipToPos.size())
		    out << del;
	    }
	    else {
		i -= colR_.size();
		if (i < colS_.size()) {
		    dumpS(out, i);
		    if (j+1 < ipToPos.size())
			out << del;
		}
	    }
	}
    }
    else {
	for (size_t j = 0; j < ipToPos.size(); ++ j) {
	    uint32_t i = ipToPos[j];
	    if (i < colR_.size()) {
		dumpR(out, i);
		if (j+1 < ipToPos.size())
		    out << ", ";
	    }
	    else {
		i -= colR_.size();
		if (i < colS_.size()) {
		    dumpS(out, i);
		    if (j+1 < ipToPos.size())
			out << ", ";
		}
	    }
	}
    }
    out << std::endl;
    return 0;
} // ibis::joinIN::result::dump

int ibis::joinIN::result::getColumnAsString(size_t cnum,
					    std::string& val) const {
    if (currR_ >= blockR_ || currS_ >= blockS_ || cnum >= ipToPos.size())
	return -1;

    int ierr = -2;
    uint32_t pos = ipToPos[cnum];
    if (pos < colR_.size()) {
	if (colR_[pos] != 0) {
	    switch (typeR_[pos]) {
	    default: break;
	    case ibis::BYTE:
		val = (*(static_cast<array_t<char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::UBYTE:
		val = (*(static_cast<array_t<unsigned char>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		ierr = 0;
		break;
	    case ibis::SHORT: {
		std::ostringstream oss;
		oss << (*(static_cast<array_t<int16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::USHORT: {
		std::ostringstream oss;
		oss << (*(static_cast<array_t<uint16_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::INT: {
		std::ostringstream oss;
		oss << (*(static_cast<array_t<int32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::UINT: {
		std::ostringstream oss;
		oss << (*(static_cast<array_t<uint32_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::ULONG: {
		std::ostringstream oss;
		oss << (*(static_cast<array_t<uint64_t>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::FLOAT: {
		std::ostringstream oss;
		oss << (*(static_cast<array_t<float>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::DOUBLE: {
		std::ostringstream oss;
		oss << (*(static_cast<array_t<double>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::TEXT:
	    case ibis::CATEGORY:
		val = (*(static_cast<std::vector<std::string>*>(valR_[pos])))
		    [(*jin_.orderR_)[currR_]];
		break;
	    }
	}
    }
    else {
	pos -= colR_.size();
	if (colS_[pos] != 0) {
	    switch (typeS_[pos]) {
	    default: break;
	    case ibis::BYTE: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::UBYTE: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<unsigned char>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::SHORT: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<int16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::USHORT: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<uint16_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::INT: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<int32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::UINT: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<uint32_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::ULONG: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<uint64_t>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::FLOAT: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<float>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::DOUBLE: {
		std::stringstream oss;
		oss << (*(static_cast<array_t<double>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		val = oss.str();
		ierr = 0;
		break;}
	    case ibis::TEXT:
	    case ibis::CATEGORY:
		val = (*(static_cast<std::vector<std::string>*>(valS_[pos])))
		    [(*jin_.orderS_)[currS_]];
		break;
	    }
	}
    }
    return ierr;
} // ibis::joinIN::result::getColumnAsString

int
ibis::joinIN::result::getColumnAsByte(const char* cname, char* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsByte((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsByte(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsByte(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsByte

int
ibis::joinIN::result::getColumnAsUByte(const char* cname,
				       unsigned char* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsUByte((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsUByte(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsUByte(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsUByte

int
ibis::joinIN::result::getColumnAsShort(const char* cname, int16_t* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsShort((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsShort(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsShort(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsShort

int
ibis::joinIN::result::getColumnAsUShort(const char* cname,
					uint16_t* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsUShort((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsUShort(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsUShort(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsUShort

int
ibis::joinIN::result::getColumnAsInt(const char* cname, int32_t* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsInt((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsInt(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsInt(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsInt

int
ibis::joinIN::result::getColumnAsUInt(const char* cname, uint32_t* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsUInt((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsUInt(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsUInt(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsUInt

int
ibis::joinIN::result::getColumnAsLong(const char* cname, int64_t* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsLong((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsLong(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsLong(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsLong

int
ibis::joinIN::result::getColumnAsULong(const char* cname, uint64_t* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsULong((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsULong(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsULong(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsULong

int
ibis::joinIN::result::getColumnAsFloat(const char* cname, float* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsFloat((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsFloat(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsFloat(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsFloat

int
ibis::joinIN::result::getColumnAsDouble(const char* cname, double* val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsDouble((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsDouble(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsDouble(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsDouble

int
ibis::joinIN::result::getColumnAsString(const char* cname,
					std::string& val) const {
    std::map<const char*, uint32_t, ibis::lessi>::const_iterator it =
	namesToPos.find(cname);
    if (it != namesToPos.end()) {
	return getColumnAsString((*it).second, val);
    }
    else {
	for (uint32_t j = 0; j < colR_.size(); ++ j) {
	    if (stricmp(cname, colR_[j]->name()) == 0)
		return getColumnAsString(j, val);
	}
	for (uint32_t j = 0; j < colS_.size(); ++ j) {
	    if (stricmp(cname, colS_[j]->name()) == 0)
		return getColumnAsString(j+colR_.size(), val);
	}
    }
    return -3;
} // ibis::joinIN::result::getColumnAsString

ibis::join* ibis::join::create(const ibis::part& partr, const ibis::part& parts,
			       const char* colname, const char* condr,
			       const char* conds) {
    return new ibis::joinIN(partr, parts, colname, condr, conds);
} // ibis::join::create
