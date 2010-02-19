// File: $Id$
// Author: John Wu <John.Wu at ACM.org>
// Copyright 2010 the Regents of the University of California
#include "jnatural.h"	// ibis::jNatural
#include "jrange.h"	// ibis::jRange
#include "filter.h"	// ibis::filter
#include "bord.h"	// ibis::bord, ibis::bord::bufferList
#include "fromClause.h"	// ibis::fromClause
#include "whereClause.h"// ibis::whereClause

/// Generate a query expression.  This function expects a valid where
/// clause, but the select clause and the from clause could be unspecified
/// or left as nil pointers.
ibis::quaere* ibis::quaere::create(const char* sel,
				   const char* fr,
				   const char* wh) {
    if (wh == 0 || *wh == 0)
	return 0;

    std::string sql;
    if (fr != 0 && *fr != 0) {
	sql += "From ";
	sql += fr;
    }
    sql += " Where ";
    sql += wh;

    try {
	ibis::whereClause wc(wh);
	if (wc.empty()) {
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- quaere::create(" << sql
		<< ") failed to parse the where clause";
	    return 0;
	}

	std::set<std::string> plist;
	wc.getExpr()->getTableNames(plist);
	if (plist.empty() || (plist.size() == 1 && plist.begin()->empty())) {
	    ibis::selectClause sc(sel);
	    if (sc.empty()) {
		return new ibis::filter(wc);
	    }
	    else {
		return new ibis::filter(wc, ibis::datasets, sc);
	    }
	}
	else if (plist.size() == 1) {
	    std::set<std::string>::const_iterator pit = plist.begin();
	    ibis::part *pt = ibis::findDataset(pit->c_str());
	    if (pt == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- quaere::create(" << sql
		    << ") can't find a data partition known as " << *pit;
		return 0;
	    }
	    else {
		ibis::selectClause sc(sel);
		ibis::partList pl(1);
		pl[0] = pt;
		return new ibis::filter(wc, pl, sc);
	    }
	}
	else if (plist.size() == 2) { // two tables
	    std::set<std::string>::const_iterator pit = plist.begin();
	    const char *pr = pit->c_str();
	    ++ pit;
	    const char *ps = pit->c_str();
	    ibis::fromClause fc(fr);

	    const char *rpr = fc.find(pr);
	    if (rpr == 0 || *rpr == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- quaere::create(" << sql
		    << ") can't find a data partition known as " << pr;
		return 0;
	    }
	    const ibis::part *partr = ibis::findDataset(rpr);
	    if (partr == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- quaere::create(" << sql
		    << ") can't find a data partition named " << rpr << " ("
		    << pr << ")";
		return 0;
	    }

	    const char *rps = fc.find(ps);
	    if (rps == 0 || *rps == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- quaere::create(" << sql
		    << ") can't find a data partition known as " << ps;
		return 0;
	    }
	    const ibis::part *parts = ibis::findDataset(rps);
	    if (parts == 0) {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- quaere::create(" << sql
		    << ") can't find a data partition named " << rps << " ("
		    << ps << ")";
		return 0;
	    }

	    ibis::qExpr *condr = 0;
	    ibis::qExpr *conds = 0;
	    ibis::qExpr *condj = 0;
	    ibis::qExpr::termTableList ttl;
	    wc.getExpr()->getConjunctiveTerms(ttl);
	    for (size_t j = 0; j < ttl.size() ; ++ j) {
		if (ttl[j].tnames.size() == 0) {
		    LOGGER(ibis::gVerbose > 1)
			<< "Warning -- quaere::create assign unqualified "
			"condition " << *(ttl[j].term) << " to " << pr;
		    if (condr != 0) {
			ibis::qExpr *tmp =
			    new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
			tmp->setLeft(condr);
			tmp->setRight(ttl[j].term->dup());
			condr = tmp;
		    }
		    else {
			condr = ttl[j].term->dup();
		    }
		}
		else if (ttl[j].tnames.size() == 1) {
		    pit = ttl[j].tnames.begin();
		    if (pit->empty()) {
			LOGGER(ibis::gVerbose > 1)
			    << "Warning -- quaere::create assign unqualified "
			    "condition " << *(ttl[j].term) << " to " << pr;
			if (condr != 0) {
			    ibis::qExpr *tmp =
				new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
			    tmp->setLeft(condr);
			    tmp->setRight(ttl[j].term->dup());
			    condr = tmp;
			}
			else {
			    condr = ttl[j].term->dup();
			}
		    }
		    else if (stricmp(pit->c_str(), pr) == 0) {
			if (condr != 0) {
			    ibis::qExpr *tmp =
				new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
			    tmp->setLeft(condr);
			    tmp->setRight(ttl[j].term->dup());
			    condr = tmp;
			}
			else {
			    condr = ttl[j].term->dup();
			}
		    }
		    else if (stricmp(pit->c_str(), ps) == 0) {
			if (conds != 0) {
			    ibis::qExpr *tmp =
				new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
			    tmp->setLeft(conds);
			    tmp->setRight(ttl[j].term->dup());
			    conds = tmp;
			}
			else {
			    conds = ttl[j].term->dup();
			}
		    }
		    else {
			LOGGER(ibis::gVerbose > 1)
			    << "Warning -- quaere::create discard condition "
			    << *(ttl[j].term) << " due to unknown name "
			    << *pit;
		    }
		}
		else if (ttl[j].tnames.size() == 2) {
		    pit = ttl[j].tnames.begin();
		    const char *tpr = pit->c_str();
		    ++ pit;
		    const char *tps = pit->c_str();
		    if (*tpr == 0) {
			const char *tmp = tpr;
			tpr = tps;
			tps = tmp;
		    }
		    if (*tps == 0) {
			if (stricmp(tpr, pr) == 0) {
			    if (condr != 0) {
				ibis::qExpr *tmp =
				    new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
				tmp->setLeft(condr);
				tmp->setRight(ttl[j].term->dup());
				condr = tmp;
			    }
			    else {
				condr = ttl[j].term->dup();
			    }
			}
			else if (stricmp(tpr, ps) == 0) {
			    if (conds != 0) {
				ibis::qExpr *tmp =
				    new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
				tmp->setLeft(conds);
				tmp->setRight(ttl[j].term->dup());
				conds = tmp;
			    }
			    else {
				conds = ttl[j].term->dup();
			    }
			}
			else {
			    LOGGER(ibis::gVerbose >= 0)
				<< "Warning -- quaere::create encountered an "
				"internal error, the where clause \"" << wh
				<< "\" is supposed to involve " << pr
				<< " and " << ps << ", but "
				<< *(ttl[j].term) << " involves table " << tpr;
			}
		    }
		    else if ((stricmp(tpr, pr) == 0 ||
			      stricmp(tpr, rpr) == 0) &&
			     (stricmp(tps, ps) == 0 ||
			      stricmp(tps, rps) == 0)) {
			if (condj != 0) {
			    ibis::qExpr *tmp =
				new ibis::qExpr(ibis::qExpr::LOGICAL_AND);
			    tmp->setLeft(condj);
			    tmp->setRight(ttl[j].term->dup());
			    condj = tmp;
			}
			else {
			    condj = ttl[j].term->dup();
			}
		    }
		    else {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warning -- quaere::create encountered an "
			    "internal error, the where clause \"" << wh
			    << "\" is supposed to involve " << pr
			    << " and " << ps << ", but "
			    << *(ttl[j].term) << " involves tables " << tpr
			    << " and " << tps;
		    }
		}
		else {
		    LOGGER(ibis::gVerbose >= 0)
			<< "Warning -- quaere::create encountered an internal "
			"error, the where clause \"" << wh
			<< "\" to said to involve 2 tables overall, but "
			"the condition " << *(ttl[j].term)
			<< " actually involves " << ttl[j].tnames.size();
		}
	    } // for (j ...

	    if (condr == 0 && conds == 0 && condj == 0) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- quaere::create(" << sql
		    << ") failed to extract any condition";
		return 0;
	    }
	    else if (condj == 0 && (condr != 0 || conds != 0)) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- quaere::create(" << sql
		    << ") will continue with " << *(condr != 0 ? condr : conds)
		    << " on " << (condr != 0 ? pr : ps);
		///*** ADD FILTER HERE!
		return 0;
	    }
	    else if (condj == 0) {
		LOGGER(ibis::gVerbose > 0)
		    << "Warning -- quaere::create(" << sql
		    << ") expects a join condition, but found none";
		return 0;
	    }
	    else if (condj->getType() == ibis::qExpr::COMPRANGE) {
		const ibis::compRange &cr =
		    *static_cast<ibis::compRange*>(condj);
		if (cr.getLeft() != 0 && cr.getRight() != 0 &&
		    cr.getTerm3() == 0 &&
		    static_cast<const ibis::math::term*>
		    (cr.getLeft())->termType()
		    == ibis::math::VARIABLE &&
		    static_cast<const ibis::math::term*>
		    (cr.getRight())->termType()
		    == ibis::math::VARIABLE) {
		    const ibis::math::variable &varr =
			*static_cast<const ibis::math::variable*>(cr.getLeft());
		    ibis::column *colr = partr->getColumn(varr.variableName());
		    if (colr == 0) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warnign -- quaere::create(" << sql
			    << ") can't find a column named "
			    << varr.variableName() << " in data partition "
			    << partr->name() << " (" << pr << ")";
			return 0;
		    }
		    const ibis::math::variable &vars =
			*static_cast<const ibis::math::variable*>
			(cr.getRight());
		    ibis::column *cols = parts->getColumn(vars.variableName());
		    if (cols == 0) {
			LOGGER(ibis::gVerbose >= 0)
			    << "Warnign -- quaere::create(" << sql
			    << ") can't find a column named "
			    << vars.variableName() << " in data partition "
			    << parts->name() << " (" << ps << ")";
			return 0;
		    }
		    return new ibis::jNatural(*partr, *parts, *colr, *cols,
					      condr, conds, sql.c_str());
		}
		else {
		    LOGGER(ibis::gVerbose > 0)
			<< "Warning -- quaere::create(" << sql
			<< ") can not handle join expression " << *condj
			<< " yet.";
		    return 0;
		}
	    }
	    else {
		LOGGER(ibis::gVerbose >= 0)
		    << "Warning -- quaere::create(" << sql
		    << ") connot process join with multiple conditions yet";
		return 0;
	    }
	}
	else { // more than two tables
	    LOGGER(ibis::gVerbose >= 0)
		<< "Warning -- quaere::create(" << sql
		<< ") does not work with more than two tables";
	    return 0;
	}
    }
    catch (std::exception &e) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- quaere::create(" << sql
	    << ") failed due to an exception -- " << e.what();
	return 0;
    }
    catch (const char* s) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- quaere::create(" << sql
	    << ") failed due to an exception -- " << s;
	return 0;
    }
    catch (...) {
	LOGGER(ibis::gVerbose > 0)
	    << "Warning -- quaere::create(" << sql
	    << ") failed due to an unexpected exception";
	return 0;
    }
} // ibis::quaere::create

/// This is equivalent to SQL statement
///
/// "From partr Join parts Using(colname) Where condr And conds"
///
/// Note that conditions specified in condr is for partr only, and conds is
/// for parts only.  If no conditions are specified, all valid records in
/// the partition will participate in the natural join.
ibis::quaere*
ibis::quaere::create(const ibis::part& partr, const ibis::part& parts,
		     const char* colname, const char* condr,
		     const char* conds) {
    return new ibis::jNatural(partr, parts, colname, condr, conds);
} // ibis::quaere::create
