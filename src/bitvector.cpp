// $Id$
// Author: John Wu <John.Wu@ACM.org> Lawrence Berkeley National Laboratory
// Copyright 2000-2008 the Regents of the University of California
//
/// @file
/// The implementation of class bitvector as defined in bitvector.h.
/// The major goal of this implementation is to avoid accessing anything
/// smaller than a word (uint32_t).
///
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable:4786)	// some identifier longer than 256 characters
#endif
#include "bitvector.h"

// constances defined in bitvector
const unsigned ibis::bitvector::MAXBITS =
8*sizeof(ibis::bitvector::word_t) - 1;
const unsigned ibis::bitvector::SECONDBIT =
ibis::bitvector::MAXBITS - 1;
const ibis::bitvector::word_t ibis::bitvector::ALLONES =
((1U << ibis::bitvector::MAXBITS) - 1);
const ibis::bitvector::word_t ibis::bitvector::MAXCNT =
((1U << ibis::bitvector::SECONDBIT) - 1);
const ibis::bitvector::word_t ibis::bitvector::FILLBIT =
(1U << ibis::bitvector::SECONDBIT);
const ibis::bitvector::word_t ibis::bitvector::HEADER0 =
(2U << ibis::bitvector::SECONDBIT);
const ibis::bitvector::word_t ibis::bitvector::HEADER1 =
(3U << ibis::bitvector::SECONDBIT);

ibis::bitvector::bitvector(const char* file) : nbits(0), nset(0) {
    try {read(file);} catch(...) {/*return empty bitvector*/}
} // ctor from file

// set a bitvector to contain n bits of val
void ibis::bitvector::set(int val, ibis::bitvector::word_t n) {
    clear(); // clear the current content
    m_vec.nosharing(); // make sure the array is not shared
    word_t k = n / MAXBITS;

    if (k > 1) {
	append_counter(val, k);
    }
    else if (k == 1) {
	if (val != 0) active.val = ALLONES;
	else active.val = 0;
	append_active();
    }

    // put the left over bits into active
    active.nbits = n - k * MAXBITS;
    if (val != 0) {
	active.val = (1U << active.nbits) - 1;
	nset = k * MAXBITS;
    }
} // ibis::bitvector::set

/// The incoming argument @c w is assumed to be a WAH compressed word.
void ibis::bitvector::appendWord(ibis::bitvector::word_t w) {
    word_t nb1, nb2;
    int cps = (w>>MAXBITS);
    nset = 0;
    if (active.nbits) { // active contains some uncompressed bits
	word_t w1;
	nb1 = active.nbits;
	nb2 = MAXBITS - active.nbits;
	active.val <<= nb2;
	if (cps != 0) { // incoming bits are comporessed
	    int b2 = (w>=HEADER1);
	    if (b2 != 0) {
		w1 = (1<<nb2)-1;
		active.val |= w1;
	    }
	    append_active();
	    nb2 = (w & MAXCNT) - 1;
	    if (nb2 > 1) {		// append a counter
		append_counter(b2, nb2);
	    }
	    else if (nb2 == 1) {
		if (b2 != 0) active.val = ALLONES;
		append_active();
	    }
	    active.nbits = nb1;
	    active.val = ((1 << nb1) - 1)*b2;
	}
	else { // incoming bits are not compressed
	    w1 = (w>>nb1);
	    active.val |= w1;
	    append_active();
	    w1 = (1<<nb1)-1;
	    active.val = (w & w1);
	    active.nbits = nb1;
	}
    } // end of the case where there are active bits
    else if (cps != 0) { // no active bit
	int b2 = (w>=HEADER1);
	nb2 = (w & MAXCNT);
	if (nb2 > 1)
	    append_counter(b2, nb2);
	else if (nb2 == 1) {
	    if (b2) active.val = ALLONES;
	    append_active();
	}
    }
    else { // no active bits
	// new word is a raw bit pattern, simply add the word
	active.val= w;
	append_active();
    }
} // ibis::bitvector::appendWord

// append another bitvector to the current
ibis::bitvector& ibis::bitvector::operator+=(const ibis::bitvector& bv) {
    if (nset>0 && bv.nset>0)
	nset += bv.nset;
    else
	nset = 0;
    word_t expbits = size() + bv.size();

    // append the words in bv.m_vec
    for (array_t<word_t>::const_iterator i=bv.m_vec.begin();
	 i!=bv.m_vec.end(); i++)
	appendWord(*i);

    // append active bits of bv
    if (active.nbits > 0) { // need to combine the two active bit sets
	if (active.nbits + bv.active.nbits < MAXBITS) {
	    // two active words can fit into one
	    active.val <<= bv.active.nbits;
	    active.val |= bv.active.val;
	    active.nbits += bv.active.nbits;
	}
	else { // two sets can not fit into one single word
	    const word_t nb1 = (active.nbits + bv.active.nbits) - MAXBITS;
	    active.val <<= (MAXBITS - active.nbits);
	    word_t w1 = (bv.active.val >> nb1);
	    active.val |= w1;
	    append_active();
	    active.nbits = nb1;
	    if (nb1 > 0)
		active.val = ((1U << nb1) - 1) & bv.active.val;
	}
    }
    else { // simply copy the active_word from bv the *this
	active.nbits = bv.active.nbits;
	active.val = bv.active.val;
    }

    if (expbits != size())
	ibis::util::logMessage("Warning", "operator+= expected %lu bits in "
			       "the resulting bitvector, but got %lu instead",
			       static_cast<long unsigned>(expbits),
			       static_cast<long unsigned>(size()));
    return *this;
} // ibis::bitvector::operator+=

/// Compress the current m_vec in-place.
void ibis::bitvector::compress() {
    if (m_vec.size() < 2 || m_vec.incore() == false) // there is nothing to do
	return;

    struct xrun {
	bool   isFill;
	int    fillBit;
	word_t nWords;
	array_t<word_t>::iterator it;

	xrun() : isFill(false), fillBit(0), nWords(0), it(0) {};
	void decode() {
	    fillBit = (*it > HEADER1);
	    isFill = (*it > ALLONES);
	    nWords = (*it & MAXCNT);
	}
    };
    xrun last;	// point to the last code word in m_vec that might be modified
                // NOTE: last.nWords is not used by this function
    xrun current;// point to the current code to be examined

    current.it = m_vec.begin();
    last.it = m_vec.begin();
    last.decode();
    for (++ current.it; current.it < m_vec.end(); ++ current.it) {
	current.decode();
	if (last.isFill) { // last word was a fill word
	    if (current.isFill) { // current word is a fill word
		if (current.fillBit == last.fillBit) { // same type of fill
		    *(last.it) += current.nWords;
		}
		else { // different types of fills, move last foward by one
		    ++ last.it;
		    *(last.it) = *(current.it);
		    last.fillBit = current.fillBit;
		}
	    }
	    else if ((last.fillBit == 0 && *(current.it) == 0) ||
		     (last.fillBit != 0 && *(current.it) == ALLONES)){
		// increase the last fill by 1 word
		++ *(last.it);
	    }
	    else  { // move last forward by one
		++ last.it;
		last.isFill = false;
		*(last.it) = *(current.it);
	    }
	}
	else if (current.isFill) {
	    // last word was a literal word, current word is a fill word
	    if ((current.fillBit == 0 && *(last.it) == 0) ||
		(current.fillBit != 0 && *(last.it) == ALLONES)) {
		// change the last word into a fill word
		*(last.it) = *(current.it) + 1;
		last.fillBit = current.fillBit;
		last.isFill = true;
	    }
	    else { // move last forward by one
		++ last.it;
		last.isFill = true;
		*(last.it) = *(current.it);
		last.fillBit = current.fillBit;
	    }
	}
	else if (*(last.it) == *(current.it)) {
	    // both last word and current word are literal words and are
	    // the same
	    if (*(current.it) == 0) { // make a 2-word 0-fill
		*(last.it) = HEADER0 | 2;
		last.isFill = true;
		last.fillBit = 0;
	    }
	    else if (*(current.it) == ALLONES) { // make a 2-word 1-fill
		*(last.it) = HEADER1 | 2;
		last.isFill = true;
		last.fillBit = 1;
	    }
	    else { // move last forward
		++ last.it;
		*(last.it) = *(current.it);
	    }
	}
	else { // move last forward one word
	    ++ last.it;
	    *(last.it) = *(current.it);
	}
    }
    ++ last.it;
    if (last.it < m_vec.end()) { // reduce the size of m_vec
	m_vec.erase(last.it, m_vec.end());
    }
} // ibis::bitvector::compress

/// Decompress the currently compressed bitvector.  Throw ibis::bad_alloc
/// exception if fails to allocate enough memory.
void ibis::bitvector::decompress() {
    if (nbits == 0 && m_vec.size() > 0)
	nbits = do_cnt();
    if (m_vec.size()*MAXBITS == nbits) // already uncompressed
	return;

    array_t<word_t> tmp;
    tmp.resize(nbits/MAXBITS);
    if (nbits != tmp.size()*MAXBITS) {
	LOGGER(1) << "Warning -- bitvector::decompress(nbits=" << nbits
		  << ") failed to allocate a temp array of "
		  << nbits/MAXBITS << "-word";
	throw ibis::bad_alloc("ibis::bitvector::decompress failed to "
			      "allocate array to uncompressed bits");
    }

    array_t<word_t>::iterator it = tmp.begin();
    array_t<word_t>::const_iterator i0 = m_vec.begin();
    for (; i0!=m_vec.end(); i0++) {
	if ((*i0) > ALLONES) {
	    word_t cnt = (*i0 & MAXCNT);
	    if ((*i0)>=HEADER1) {
		for (word_t j=0; j<cnt; j++, it++)
		    *it = ALLONES;
	    }
	    else {
		for (word_t j=0; j<cnt; j++, it++)
		    *it = 0;
	    }
	}
	else {
	    *it = *i0;
	    it++;
	}
    }

    if (m_vec.size() != tmp.size())
	m_vec.swap(tmp);  // take on the new vector
} // ibis::bitvector::decompress

// decompress the current content to an array_t<word_t>
void ibis::bitvector::decompress(array_t<ibis::bitvector::word_t>& tmp)
    const {
    const word_t nb = ((nbits == 0 && m_vec.size() > 0) ? do_cnt() : nbits);
    word_t cnt = nb/MAXBITS;
    tmp.resize(cnt);

    array_t<word_t>::iterator it = tmp.begin();
    array_t<word_t>::const_iterator i0 = m_vec.begin();
    for (; i0!=m_vec.end(); i0++) {
	if ((*i0) > ALLONES) {
	    cnt = (*i0 & MAXCNT);
	    if ((*i0)>=HEADER1) {
		for (word_t j=0; j<cnt; j++, it++)
		    *it = ALLONES;
	    }
	    else {
		for (word_t j=0; j<cnt; j++, it++)
		    *it = 0;
	    }
	}
	else {
	    *it = *i0;
	    it++;
	}
    }
} // ibis::bitvector::decompress

// decompress the current content to an array_t<word_t> and complement every
// bit
void ibis::bitvector::copy_comp(array_t<ibis::bitvector::word_t>& tmp) const {
    word_t cnt = (nbits == 0 && m_vec.size()>0 ? do_cnt() : nbits)/MAXBITS;
    tmp.resize(cnt);

    array_t<word_t>::iterator it = tmp.begin();
    array_t<word_t>::const_iterator i0 = m_vec.begin();
    for (; i0!=m_vec.end(); i0++) {
	if ((*i0) > ALLONES) {
	    cnt = (*i0 & MAXCNT);
	    if ((*i0)>=HEADER1) {
		for (word_t j=0; j<cnt; j++, it++)
		    *it = 0;
	    }
	    else {
		for (word_t j=0; j<cnt; j++, it++)
		    *it = ALLONES;
	    }
	}
	else {
	    *it = ALLONES ^ *i0;
	    it++;
	}
    }
} // ibis::bitvector::copy_comp

// determine the number of word that would be eliminated by compression
ibis::bitvector::word_t ibis::bitvector::compressible() const {
    word_t cnt=0;
    for (word_t i=0; i<m_vec.size()-1; i++) {
	cnt += ((m_vec[i] == m_vec[i+1]) &&
		((m_vec[i] == 0) || (m_vec[i] == ALLONES)));
    }
    return cnt;
} // ibis::bitvector::compressible

// return the number of bits counted and modify the member variable nset to
// the correct value
ibis::bitvector::word_t ibis::bitvector::do_cnt() const throw() {
    nset = 0;
    word_t nb = 0;
    array_t<word_t>::const_iterator i;

    for (i = m_vec.begin(); i < m_vec.end(); ++i) {
	if ((*i) < HEADER0) {
	    nb += MAXBITS;
	    nset += cnt_ones(*i);
	}
	else {
	    word_t tmp = (*i & MAXCNT) * MAXBITS;
	    nb += tmp;
	    nset += tmp * ((*i) >= HEADER1);
	}
    }
    return nb;
} // ibis::bitvector::do_cnt

// replace the ind'th bit with val.  val is assumed to be either 0 or 1.  If
// val is not 0 or 1, it could cause serious problems.
// This functioin can be used to extend the length of the bit sequence.
// When the given index (ind) is beyond the end of the current sequence, the
// unspecified bits in the range of [size(), ind) are assumed to be 0.
void ibis::bitvector::setBit(const ibis::bitvector::word_t ind, int val) {
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "ibis::bitvector::setBit(" << ind << ", " << val << ") "
		  << "-- " << nbits << " bit(s) in m_vec and "
		  << active.nbits << " bit(s) in the active word";
    }
#endif
    if (ind >= size()) {
	word_t diff = ind - size() + 1;
	if (active.nbits) {
	    if (ind+1 >= nbits+MAXBITS) {
		diff -= MAXBITS - active.nbits;
		active.val <<= (MAXBITS - active.nbits);
		if (diff == 0)
		    active.val += (val!=0);
		append_active();
	    }
	    else {
		active.nbits += diff;
		active.val <<= diff;
		active.val += (val!=0);
		diff = 0;
	    }
	}
	if (diff) {
	    word_t w = diff / MAXBITS;
	    diff -= w * MAXBITS;
	    if (diff) {
		if (w > 1) {
		    append_counter(0, w);
		}
		else if (w) {
		    append_active();
		}
		active.nbits = diff;
		active.val += (val!=0);
	    }
	    else if (val != 0) {
		if (w > 2) {
		    append_counter(0, w-1);
		}
		else if (w == 2) {
		    append_active();
		}
		active.val = 1;
		append_active();
	    }
	    else {
		if (w > 1) {
		    append_counter(0, w);
		}
		else if (w) {
		    append_active();
		}
	    }
	}
	if (size() != ind+1)
	    ibis::util::logMessage("Warning", "bitvector::setBit(%lu, %d) "
				   "changed bitvector size() to %lu, but "
				   "%lu was expected",
				   static_cast<long unsigned>(ind), val,
				   static_cast<long unsigned>(ind+1),
				   static_cast<long unsigned>(size()));
	if (nset)
	    nset += (val!=0);
	return;
    }
    else if (ind >= nbits) { // modify an active bit
	word_t u = active.val;
	if (val != 0) {
	    active.val |= (1 << (active.nbits - (ind - nbits) - 1));
	}
	else {
	    active.val &= ~(1 << (active.nbits - (ind - nbits) - 1));
	}
	if (nset && (u != active.val))
	    nset += (val?1:-1);
	return;
    }
    else if (m_vec.size()*MAXBITS == nbits) { // uncompressed
	const word_t i = ind / MAXBITS;
	const word_t u = m_vec[i];
	const word_t w = (1 << (SECONDBIT - (ind % MAXBITS)));
	if (val != 0)
	    m_vec[i] |= w;
	else
	    m_vec[i] &= ~w;
	if (nset && (m_vec[i] != u))
	    nset += (val?1:-1);
	return;
    }

    // normal case, compressed bit vector -- 
    // the bit to be modified is in m_vec
    array_t<word_t>::iterator it = m_vec.begin();
    word_t compressed = 0, cnt = 0, ind1 = 0, ind0 = ind;
    int current = 0; // current bit value
    while ((ind0>0) && (it<m_vec.end())) {
	if (*it >= HEADER0) { // a fill
	    cnt = ((*it) & MAXCNT) * MAXBITS;
	    if (cnt > ind0) { // found the location
		current = (*it >= HEADER1);
		compressed = 1;
		ind1 = ind0;
		ind0 = 0;
	    }
	    else {
		ind0 -= cnt;
		ind1 = ind0;
		++ it;
	    }
	}
	else { // a literal word
	    cnt = MAXBITS;
	    if (MAXBITS > ind0) { // found the location
		current = (1 & ((*it) >> (SECONDBIT - ind0)));
		compressed = 0;
		ind1 = ind0;
		ind0 = 0;
	    }
	    else {
		ind0 -= MAXBITS;
		ind1 = ind0;
		++ it;
	    }
	}
    } // while (ind...
    if (ind1 == 0) { // set current and compressed
	if (*it >= HEADER0) {
	    cnt = ((*it) & MAXCNT) * MAXBITS;
	    current = (*it >= HEADER1);
	    compressed = 1;
	}
	else {
	    cnt = MAXBITS;
	    current = (*it >> SECONDBIT);
	    compressed = 0;
	}
    }

    if (ind0>0) { // has not found the right location yet
	ibis::util::logMessage("Warning", "bitvector::setBit(%lu, %d) passed "
			       "the end (%lu) of bit sequence while searching "
			       "for position %lu",
			       static_cast<long unsigned>(ind), val,
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(ind));
	if (ind0 < active.nbits) { // in the active word
	    ind1 = (1 << (active.nbits - ind0 - 1));
	    if (val != 0) {
		active.val |= ind1;
	    }
	    else {
		active.val &= ~ind1;
	    }
	}
	else { // extends the current bit vector
	    ind1 = ind0 - active.nbits - 1;
	    appendWord(HEADER0 | (ind1/MAXBITS));
	    for (ind1%=MAXBITS; ind1>0; --ind1) operator+=(0);
	    operator+=(val != 0);
	}
	if (nset) nset += val?1:-1;
	return;
    }

    // locate the bit to be changed, lots of work hidden here
    if (current == val) return; // nothing to do

    // need to actually modify the bit
    if (compressed == 0) { // toggle a single bit of a literal word
	*it ^= (1 << (SECONDBIT - ind1));
    }
    else if (ind1 < MAXBITS) {
	// bit to be modified is in the first word, two pieces
	-- (*it);
	if ((*it & MAXCNT) == 1)
	    *it = (current)?ALLONES:0;
	word_t w = 1 << (SECONDBIT-ind1);
	if (val == 0) w ^= ALLONES;
	it = m_vec.insert(it, w);
    }
    else if (cnt - ind1 <= MAXBITS) {
	// bit to be modified is in the last word, two pieces
	-- (*it);
	if ((*it & MAXCNT) == 1)
	    *it = (current)?ALLONES:0;
	word_t w = 1 << (cnt-ind1-1);
	if (val == 0) w ^= ALLONES;
	++ it;
	it = m_vec.insert(it, w);
    }
    else { // the counter breaks into three pieces
	word_t u[2], w;
	u[0] = ind1 / MAXBITS;
	w = (*it & MAXCNT) - u[0] - 1;
	u[1] = 1 << (SECONDBIT-ind1+u[0]*MAXBITS);
	if (val==0) {
	    u[0] = (u[0]>1)?(HEADER1|u[0]):(ALLONES);
	    u[1] ^= ALLONES;
	    w = (w>1)?(HEADER1|w):(ALLONES);
	}
	else {
	    u[0] = (u[0]>1)?(HEADER0|u[0]):static_cast<word_t>(0);
	    w = (w>1)?(HEADER0|w):static_cast<word_t>(0);
	}
	*it = w;
	m_vec.insert(it, u, u+2);
    }
    if (nset)
	nset += val?1:-1;
} // ibis::bitvector::setBit

/// Select a subset of the bits.  Bits whose positions are marked 1 are put
/// together in a new bitvector @c res.
void ibis::bitvector::subset(const ibis::bitvector& mask,
			     ibis::bitvector& res) const {
    res.clear();
    if (size() == 0 || mask.cnt()== 0) return;
    if (all0s() && active.val == 0) { // res is all 0
	res.set(0, mask.cnt());
	return;
    }
    else if (all1s() && active.val+1 == (1U << active.nbits)) {
	res.set(1, mask.cnt());
	return;
    }
    else if (mask.size() != size()) {
	ibis::util::logMessage("Warning", "function subset requires mask to "
			       "be the same size as *this (# bits = %lu)",
			       static_cast<long unsigned>(size()));
	return;
    }

    run cur, sel;
    cur.it = m_vec.begin();
    cur.decode();
    for (sel.it = mask.m_vec.begin(); sel.it != mask.m_vec.end(); ++ sel.it) {
	sel.decode();
	if (sel.isFill != 0) { // current word in mask is a fill
	    if (sel.fillBit != 0) { // 1-fill: copy bits
		while (sel.nWords > 0) {
		    if (cur.nWords == 0) {
			++ cur.it;
			cur.decode();
		    }
		    if (cur.isFill != 0) {
			if (sel.nWords >= cur.nWords) {
			    res.appendFill(cur.fillBit, cur.nWords*MAXBITS);
			    sel.nWords -= cur.nWords;
			    cur.nWords = 0;
			}
			else {
			    res.appendFill(cur.fillBit, sel.nWords*MAXBITS);
			    cur.nWords -= sel.nWords;
			    sel.nWords = 0;
			}
		    }
		    else {
			res.appendWord(* cur.it);
			cur.nWords = 0;
			-- sel.nWords;
		    }
		} // while (sel.nWords > 0)
	    }
	    else { // skip sel.nWords
		while (sel.nWords > 0) {
		    if (cur.nWords == 0) {
			++ cur.it;
			cur.decode();
		    }
		    if (cur.isFill != 0) {
			if (sel.nWords >= cur.nWords) {
			    sel.nWords -= cur.nWords;
			    cur.nWords = 0;
			}
			else {
			    cur.nWords -= sel.nWords;
			    sel.nWords = 0;
			}
		    }
		    else {
			cur.nWords = 0;
			-- sel.nWords;
		    }
		}
	    }
	}
	else { // need to tease out a few bits
	    if (cur.nWords == 0) {
		++ cur.it;
		cur.decode();
	    }
	    if (*sel.it == 0) {
		if (cur.isFill != 0)
		    -- cur.nWords;
		else
		    cur.nWords = 0;
	    }
	    else if (*sel.it == ALLONES) {
		if (cur.isFill != 0) {
		    res.appendFill(cur.fillBit, MAXBITS);
		    -- cur.nWords;
		}
		else {
		    res.appendWord(*cur.it);
		    cur.nWords = 0;
		}
	    }
	    else if (cur.isFill != 0) {
		res.appendFill(cur.fillBit, cnt_ones(*sel.it));
		-- cur.nWords;
	    }
	    else if (*cur.it == 0 || *cur.it == ALLONES) {
		res.appendFill(*cur.it == ALLONES, cnt_ones(*sel.it));
		cur.nWords = 0;
	    }
	    else { // one bit at a time
		word_t mk = (*sel.it << 1);
		word_t ck = (*cur.it << 1);
		while (mk > 0) {
		    if (mk > ALLONES)
			res += (ck > ALLONES);
		    mk <<= 1;
		    ck <<= 1;
		}
		cur.nWords = 0;
	    }
	}
    } // for (sel.it ...

    if (mask.active.val > 0) { // deal with the active words
	word_t mk = (mask.active.val << (MAXBITS - active.nbits + 1));
	word_t ck = (active.val << (MAXBITS - active.nbits + 1));
	while (mk > 0) {
	    if (mk > ALLONES)
		res += (ck > ALLONES);
	    mk <<= 1;
	    ck <<= 1;
	}
    }
} // ibis::bitvector::subset

// erase the bit in the range of [i, j)
void ibis::bitvector::erase(ibis::bitvector::word_t i,
			    ibis::bitvector::word_t j) {
    if (i >= j) {
	return;
    }

    ibis::bitvector res;
    if (i > 0) { // copy the leading part to res
	iterator ip = begin();
	ip += i;
	array_t<word_t>::const_iterator cit = m_vec.begin();
	while (cit < ip.it) {
	    res.m_vec.push_back(*cit);
	    ++ cit;
	}
	res.nbits = i - ip.ind;
	if (ip.compressed) {
	    for (word_t ii=0; ii<ip.ind; ++ii) res += ip.fillbit;
	}
	else {
	    res.active.val = (ip.literalvalue >> (MAXBITS-ip.ind));
	    res.active.nbits = ip.ind;
	}
    }

    if (j < nbits) { // need to copy something from m_vec
	iterator iq = begin();
	iq += j;
	// copy second half of iq.it
	if (iq.compressed) {
	    for (word_t ii=iq.ind; ii<iq.nbits; ++ii) {
		res += iq.fillbit;
	    }
	}
	else {
	    for (long ii=(iq.nbits-iq.ind-1); ii>=0; --ii) {
		res += (1 & (iq.literalvalue >> ii));
	    }
	}
	// copy the remaining whole words
	++ (iq.it);
	while (iq.it != m_vec.end()) {
	    res.appendWord(*(iq.it));
	    ++ (iq.it);
	}
	// copy the active word
	for (long ii=(active.nbits-1); ii>=0; --ii) {
	    res += (int) ((active.val >> ii) & 1);
	}
    }
    else if (j < nbits+active.nbits) { // only something from active word
	for (long ii=(active.nbits-j+nbits-1); ii>=0; --ii) {
	    res += (int) ((active.val >> ii) & 1);
	}
    }
    swap(res);
} // ibis::bitvector::erase

/// Count the number of bits that are 1 that also marked 1.  A
/// straightforward implement of this is to perform a bitwise AND and then
/// count the number of bits that are 1.  However, such an approach will
/// generate a bitvector that is only used for counting.  This is an
/// attempt to do better.
ibis::bitvector::word_t
ibis::bitvector::count(const ibis::bitvector& mask) const {
    word_t cnt = 0;
    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (mask.m_vec.size()*MAXBITS == mask.nbits);
    if (ca && cb) {
	array_t<word_t>::const_iterator j = m_vec.begin();
	array_t<word_t>::const_iterator k = mask.m_vec.begin();
	while (j < m_vec.end()) {
	    cnt += cnt_ones(*j & *k); ++j; ++k;
	}
	cnt += cnt_ones(active.val & mask.active.val);
    }
    else if (ca) {
	cnt = mask.count_c1(*this);
    }
    else if (cb) {
	cnt = count_c1(mask);
    }
    else if (all0s() || mask.all0s()) { // no 1s
	cnt = cnt_ones(active.val & mask.active.val);
    }
    else if (all1s()) { // m_vec contain only a sinle 1-fill
	if (mask.nset == 0 && ! mask.m_vec.empty())
	    mask.nbits = mask.do_cnt();
	cnt = mask.nset + cnt_ones(active.val & mask.active.val);
    }
    else if (mask.all1s()) {
	if (nbits == 0 && ! m_vec.empty())
	    nbits = do_cnt();
	cnt = nset + cnt_ones(active.val & mask.active.val);
    }
    else {
	cnt = count_c2(mask);
    }
    return cnt;
} // ibis::bitvector::count

// toggle every bit of the bit sequence
void ibis::bitvector::flip() {
    m_vec.nosharing(); // make sure *this is not shared!
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) > bytes()) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "Before flipping:" << *this;
    }
#endif
    // toggle those words in m_vec
    if (nbits > 0) {
	for (array_t<word_t>::iterator i=m_vec.begin(); i!=m_vec.end(); i++) {
	    if (*i > ALLONES) {
		*i ^= FILLBIT;
	    }
	    else {
		*i ^= ALLONES;
	    }
	}
    }
    else {
	nbits = 0;
	for (array_t<word_t>::iterator i=m_vec.begin(); i!=m_vec.end(); i++) {
	    if (*i > ALLONES) {
		*i ^= FILLBIT;
		nbits += MAXBITS;
	    }
	    else {
		*i ^= ALLONES;
		nbits += MAXBITS * (*i & MAXCNT);
	    }
	}
    }
    if (nset > 0)
	nset = nbits - nset;

    if (active.nbits > 0) { // also need to toggle active_word
	active.val ^= ((1<<active.nbits) - 1);
    }
#if defined(DEBUG)
    word_t nb = do_cnt();
    if (nb != nbits)
	ibis::util::logMessage("Warning", "ibis::bitvector::flip() expects "
			       "to have %lu bits but got %lu",
			       static_cast<long unsigned>(nbits),
			       static_cast<long unsigned>(nb));
    if (ibis::gVerbose > 30 || (1U << ibis::gVerbose) > bytes()) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "After flipping:" << *this;
    }
#endif
} // ibis::bitvector::flip

// compare two bitvectors
int ibis::bitvector::operator==(const ibis::bitvector& rhs) const {
    if (nbits != rhs.nbits) return 0;
    if (m_vec.size() != rhs.m_vec.size()) return 0;
    if (active.val != rhs.active.val) return 0;
    for (word_t i=0; i<m_vec.size(); i++)
	if (m_vec[i] != rhs.m_vec[i]) return 0;

    return 1;
} // ibis::bitvector::operator==

/// The in-place version of the bitwise logical AND operator.  It performs
/// the bitwise logical AND operation between this bitvector and @c rhs,
/// then stores the result back to this bitvector.
///
///@note If the two bit vectors are not of the same length, the shorter one
/// is implicitly padded with 0 bits so the two are of the same length.
void ibis::bitvector::operator&=(const ibis::bitvector& rhs) {
#if defined(WAH_CHECK_SIZE)
    if (nbits == 0)
	nbits = do_cnt();
    if ((rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator&= "
			       "can not operate on two bitvector of "
			       "different sizes (%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator&=: A=" << *this << "B=" << rhs;
    }
#endif

    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca) {
	if (cb)
	    and_c0(rhs);
	else
	    and_d1(rhs);
    }
    else if (cb) {
	bitvector tmp;
	tmp.copy(rhs);
	swap(tmp);
	and_d1(tmp);
    }
    else if (all0s() || rhs.all1s()) { // nothing to do
	active.val &= rhs.active.val;
    }
    else if (rhs.all0s() || all1s()) { // copy rhs
	nset = rhs.nset;
	m_vec.copy(rhs.m_vec);
	active.val &= rhs.active.val;
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS >= rhs.nbits) {
	// if the total size of the two operands are large, generate an
	// decompressed solution
	bitvector res;
	and_d2(rhs, res);
	swap(res);
    }
    else {
	bitvector res;
	and_c2(rhs, res);
	swap(res);
    }
#if defined(DEBUG)
    word_t nb = do_cnt();
    if (nbits == 0)
	nbits = nb;
    if (nb != rhs.nbits && rhs.nbits > 0)
	ibis::util::logMessage("Warning", "ibis::bitvector::operator&=() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator&=: (A&B)=" << *this;
    }
#endif
} // ibis::bitvector::operator&=

/// This version of bitwise logical operator produces a new bitvector as
/// the result and return a pointer to the new object.
///
/// @note The caller is responsible for deleting the bitvector returned.
///
///@sa ibis::bitvector::operator&=
ibis::bitvector* ibis::bitvector::operator&(const ibis::bitvector& rhs)
    const {
#if defined(WAH_CHECK_SIZE)
    if ((nbits > 0 && rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator& can not "
			       "operate on two bitvector of different sizes "
			       "(%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
    ibis::bitvector *res = new ibis::bitvector;
    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca && cb) {
	res->m_vec.resize(m_vec.size());
	array_t<word_t>::iterator i = res->m_vec.begin();
	array_t<word_t>::const_iterator j = m_vec.begin();
	array_t<word_t>::const_iterator k = rhs.m_vec.begin();
	while (i != res->m_vec.end()) {
	    *i = *j & *k; ++i; ++j; ++k;
	}
	res->active.val = active.val & rhs.active.val;
	res->active.nbits = active.nbits;
	res->nbits = nbits;
    }
    else if (ca) {
	rhs.and_c1(*this, *res);
    }
    else if (cb) {
	and_c1(rhs, *res);
    }
    else if (all0s() || rhs.all1s()) { // copy *this
	res->copy(*this);
	res->active.val &= rhs.active.val;
    }
    else if (all1s() || rhs.all0s()) { // copy rhs
	res->copy(rhs);
	res->active.val &= active.val;
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS > nbits) {
	and_d2(rhs, *res);
    }
    else {
	and_c2(rhs, *res);
    }
#if defined(DEBUG)
    word_t nb = res->do_cnt();
    if (nb != rhs.nbits && rhs.nbits > 0)
	ibis::util::logMessage("Warning", "ibis::bitvector::operator&() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
#endif
    return res;
} // ibis::bitvector::operator&

/// The is the in-place version of the bitwise OR (|) operator.  This
/// bitvector is modified to store the result of the operation.
///
///@sa ibis::bitvector::operator&=
void ibis::bitvector::operator|=(const ibis::bitvector& rhs) {
#if defined(WAH_CHECK_SIZE)
    if (nbits == 0)
	nbits = do_cnt();
    if ((rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator|= cannot "
			       "operate on two bitvector of different sizes "
			       "(%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator|=: A=" << *this << "B=" << rhs;
    }
#endif

    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca) {
	if (cb)
	    or_c0(rhs);
	else
	    or_d1(rhs);
    }
    else if (cb) {
	bitvector tmp;
	tmp.copy(rhs);
	swap(tmp);
	or_d1(tmp);
    }
    else if (all1s() || rhs.all0s()) {
	active.val |= rhs.active.val;
    }
    else if (all0s() || rhs.all1s()) {
	nset = rhs.nset;
	m_vec.copy(rhs.m_vec);
	active.val |= rhs.active.val;
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS >= rhs.nbits) {
	// if the total size of the two operands are large, generate an
	// uncompressed result
	bitvector res;
	or_d2(rhs, res);
	swap(res);
    }
    else {
	bitvector res;
	or_c2(rhs, res);
	swap(res);
    }
#if defined(DEBUG)
    word_t nb = do_cnt();
    if (nbits == 0)
	nbits = nb;
    if (nb != rhs.nbits && rhs.nbits > 0) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator|=() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
    }
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator|=: (A|B)=" << *this;
    }
#endif
} // ibis::bitvector::operator|=

/// This bitvector is not modified, instead a new bitvector is generated.
///
///@sa ibis::bitvector::operator&
ibis::bitvector* ibis::bitvector::operator|(const ibis::bitvector& rhs)
    const {
#if defined(WAH_CHECK_SIZE)
    if ((nbits > 0 && rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator| can not "
			       "operate on two bitvector of different sizes "
			       "(%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
    ibis::bitvector *res = new ibis::bitvector;
    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca && cb) {
	res->m_vec.resize(m_vec.size());
	array_t<word_t>::iterator i = res->m_vec.begin();
	array_t<word_t>::const_iterator j = m_vec.begin();
	array_t<word_t>::const_iterator k = rhs.m_vec.begin();
	while (i != res->m_vec.end()) {
	    *i = *j | *k; ++i; ++j; ++k;
	}
	res->active.val = active.val | rhs.active.val;
	res->active.nbits = active.nbits;
	res->nbits = nbits;
    }
    else if (ca) {
	rhs.or_c1(*this, *res);
    }
    else if (cb) {
	or_c1(rhs, *res);
    }
    else if (all1s() || rhs.all0s()) {
	res->copy(*this);
	res->active.val |= rhs.active.val;
    }
    else if (all0s() || rhs.all1s()) {
	res->copy(rhs);
	res->active.val |= active.val;
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS > nbits) {
	or_d2(rhs, *res);
    }
    else {
	or_c2(rhs, *res);
    }
#if defined(DEBUG)
    word_t nb = res->do_cnt();
    if (nb != rhs.nbits && rhs.nbits > 0)
	ibis::util::logMessage("Warning", "ibis::bitvector::operator|() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
#endif
    return res;
} // ibis::bitvector::operator|

/// The in-place version of the bitwise XOR (^) operator.  This bitvector
/// is modified to store the result.
///
///@sa ibis::bitvector::operator&=
void ibis::bitvector::operator^=(const ibis::bitvector& rhs) {
#if defined(WAH_CHECK_SIZE)
    if (nbits == 0)
	nbits = do_cnt();
    if ((rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator^= cannot "
			       "operate on two bitvector of different sizes "
			       "(%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator^=: A=" << *this << "B=" << rhs;
    }
#endif

    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca) {
	if (cb)
	    xor_c0(rhs);
	else
	    xor_d1(rhs);
    }
    else if (cb) {
	bitvector res;
	xor_c1(rhs, res);
	swap(res);
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS >= rhs.nbits) {
	// if the total size of the two operands are large, generate an
	// uncompressed result
	bitvector res;
	xor_d2(rhs, res);
	swap(res);
    }
    else {
	bitvector res;
	xor_c2(rhs, res);
	swap(res);
    }
#if defined(DEBUG)
    word_t nb = do_cnt();
    if (nbits == 0)
	nbits = nb;
    if (nb != rhs.nbits && rhs.nbits > 0)
	ibis::util::logMessage("Warning", "ibis::bitvector::operator^=() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator^=: (A^B)=" << *this;
    }
#endif
} // ibis::bitvector::operator^=

/// This bitvector is not modified, instead a new bitvector object is
/// generated to store the result.
///
///@sa ibis::bitvector::operator&
ibis::bitvector* ibis::bitvector::operator^(const ibis::bitvector& rhs)
    const {
#if defined(WAH_CHECK_SIZE)
    if ((nbits > 0 && rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator^ can not "
			       "operate on two bitvector of different sizes "
			       "(%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
    ibis::bitvector *res = new ibis::bitvector;
    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca && cb) {
	res->m_vec.resize(m_vec.size());
	array_t<word_t>::iterator i = res->m_vec.begin();
	array_t<word_t>::const_iterator j = m_vec.begin();
	array_t<word_t>::const_iterator k = rhs.m_vec.begin();
	while (i != res->m_vec.end()) {
	    *i = *j ^ *k; ++i; ++j; ++k;
	}
	res->active.val = active.val ^ rhs.active.val;
	res->active.nbits = active.nbits;
	res->nbits = nbits;
    }
    else if (ca) {
	rhs.xor_c1(*this, *res);
    }
    else if (cb) {
	xor_c1(rhs, *res);
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS > nbits) {
	xor_d2(rhs, *res);
    }
    else {
	xor_c2(rhs, *res);
    }
#if defined(DEBUG)
    word_t nb = res->do_cnt();
    if (nb != rhs.nbits && rhs.nbits > 0)
	ibis::util::logMessage("Warning", "ibis::bitvector::operator^() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
#endif
    return res;
} // ibis::bitvector::operator^

/// The in-place version of the bitwise minus (-) operator.  This bitvector
///is modified to store the result of the operation.
///
///@sa ibis::bitvector::operator&=
void ibis::bitvector::operator-=(const ibis::bitvector& rhs) {
#if defined(WAH_CHECK_SIZE)
    if (nbits == 0)
	nbits = do_cnt();
    if ((rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator-= can "
			       "not operate on two bitvector of different "
			       "sizes (%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
#if defined(DEBUG)
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator-=: A=" << *this << "B=" << rhs;
    }
#endif
    const bool ca = (m_vec.size()*MAXBITS == nbits);
    const bool cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca) {
	if (cb)
	    minus_c0(rhs);
	else
	    minus_d1(rhs);
    }
    else if (cb) {
	bitvector res;
	minus_c1(rhs, res);
	swap(res);
    }
    else if (all0s() || rhs.all0s()) { // keep *this
	active.val &= ~(rhs.active.val);
    }
    else if (rhs.all1s()) { // zero out m_vec
	nset = 0;
	nbits = 0;
	m_vec.clear();
	active.val &= ~(rhs.active.val);
	append_counter(0, rhs.m_vec[0]&MAXCNT);
    }
    else if (all1s()) { // ~rhs
	word_t tmp = active.val;
	copy(rhs);
	flip();
	active.val &= tmp;
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS >= rhs.nbits) {
	// if the total size of the two operands are large, generate an
	// uncompressed result
	bitvector res;
	minus_d2(rhs, res);
	swap(res);
    }
    else {
	bitvector res;
	minus_c2(rhs, res);
	swap(res);
    }
#if defined(DEBUG)
    word_t nb = do_cnt();
    if (nbits == 0)
	nbits = nb;
    if (nb != rhs.nbits && rhs.nbits > 0)
	ibis::util::logMessage("Warning", "ibis::bitvector::operator-=() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
    if (ibis::gVerbose > 30 || ((1U << ibis::gVerbose) >= bytes() &&
				(1U << ibis::gVerbose) >= rhs.bytes())) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operator-=: (A-B)=" << *this;
    }
#endif
} // ibis::bitvector::operator-=

/// The operands of the bitwise minus operation are not modified, instead a
/// new bitvector object is gnerated.
///
///@sa ibis::bitvector::operator&
ibis::bitvector* ibis::bitvector::operator-(const ibis::bitvector& rhs)
    const {
#if defined(WAH_CHECK_SIZE)
    if ((nbits > 0 && rhs.nbits > 0 && nbits != rhs.nbits) ||
	active.nbits != rhs.active.nbits) {
	ibis::util::logMessage("Warning", "ibis::bitvector::operator- can not "
			       "operate on two bitvector of different sizes "
			       "(%lu != %lu)",
			       static_cast<long unsigned>(size()),
			       static_cast<long unsigned>(rhs.size()));
    }
#endif
    ibis::bitvector *res = new ibis::bitvector;
    int ca = (m_vec.size()*MAXBITS == nbits);
    int cb = (rhs.m_vec.size()*MAXBITS == rhs.nbits);
    if (ca && cb) {
	res->m_vec.resize(m_vec.size());
	array_t<word_t>::iterator i = res->m_vec.begin();
	array_t<word_t>::const_iterator j = m_vec.begin();
	array_t<word_t>::const_iterator k = rhs.m_vec.begin();
	while (i != res->m_vec.end()) {
	    *i = *j & ~(*k); ++i; ++j; ++k;
	}
	res->active.val = active.val & ~(rhs.active.val);
	res->active.nbits = active.nbits;
	res->nbits = nbits;
    }
    else if (ca) {
	minus_c1x(rhs, *res);
    }
    else if (cb) {
	minus_c1(rhs, *res);
    }
    else if (all0s() || rhs.all0s()) { // copy *this
	res->copy(*this);
	res->active.val &= ~(rhs.active.val);
    }
    else if (rhs.all1s()) { // generate 0
	res->append_counter(0, rhs.m_vec[0]&MAXCNT);
	res->active.nbits = active.nbits;
	res->active.val = active.val & ~(rhs.active.val);
    }
    else if (all1s()) { // ~rhs
	res->copy(rhs);
	res->flip();
	res->active.val &= active.val;
    }
    else if ((m_vec.size()+rhs.m_vec.size())*MAXBITS > nbits) {
	minus_d2(rhs, *res);
    }
    else {
	minus_c2(rhs, *res);
    }
#if defined(DEBUG)
    word_t nb = res->do_cnt();
    if (nb != rhs.nbits && rhs.nbits > 0)
	ibis::util::logMessage("Warning", "ibis::bitvector::operator-() "
			       "expects to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
#endif
    return res;
} // ibis::bitvector::operator-

/// Print each word in bitvector on a line.
std::ostream& ibis::bitvector::print(std::ostream& o) const {
    if (! m_vec.empty()) {
	if (nbits == 0) {
	    nbits = do_cnt();
	}
	else if (nset == 0) {
	    word_t nb = do_cnt();
	    if (nbits != nb && nbits > 0)
		LOGGER(0) << "FastBit::bitvector::print detected nbits ("
			  << nbits << ") mismatching return value of do_cnt ("
			  << nb << "), use the return value of do_cnt";
	}
    }
    if (nbits == 0 && ! m_vec.empty())
	nbits = do_cnt();
    o << "\nThis bitvector stores " << nbits << " bits of a " << size()
      << "-bit (" << nset+cnt_ones(active.val) << " set) sequence in a "
      << m_vec.size() << "-word array and ";
    if (active.nbits==0)
	o << "zero bit in the active word" << std::endl;
    else if (active.nbits==1)
	o << "one bit in the active word" << std::endl;
    else
	o << static_cast<int>(active.nbits) << " bits in the active word"
	  << std::endl;
    if (size() == 0)
	return o;
#if defined(DEBUG)
    if (ibis::gVerbose > 6)
	m_vec.printStatus(o);
#else
    if (ibis::gVerbose > 16)
	m_vec.printStatus(o);
#endif

    // print header
    o << "\t\t\t     0    1    1    2    2    3" << std::endl;
    o << "\t\t\t0123456789012345678901234567890" << std::endl;
    o << "\t\t\t-------------------------------" << std::endl;

    // print all words in m_vec
    array_t<word_t>::const_iterator i;
    word_t j, k;
    for (i=m_vec.begin(), k=0; i!=m_vec.end(); ++i,++k) {
	o << k << "\t" << std::hex << std::setw(8) << std::setfill('0')
	  << (*i) << std::dec << "\t";
	if (*i > ALLONES) {
	    o << (*i & MAXCNT)*MAXBITS << "*" << ((*i)>=HEADER1);
	}
	else {
	    for (j=0; j<MAXBITS; j++)
		o << ((*i>>(SECONDBIT-j))&1);
	}
	o << std::endl;
    }

    if (active.nbits>0) {    // print the active_word
	o << "\t" << std::hex << std::setw(8) << std::setfill('0')
	  << (active.val << (MAXBITS-active.nbits))
	  << std::dec << "\t";
	for (j=0; j<active.nbits; j++)
	    o << (1 & (active.val>>(active.nbits-j-1)));
	//for (; j<MAXBITS; j++) o << ' ';
    }
    o << std::endl;

    return o;
} // ostream& ibis::bitvector::print(ostream& o) const

std::ostream& operator<<(std::ostream& o, const ibis::bitvector& b) {
    return b.print(o);
}

// read vector from file (purge current contents first)
// minimal amount of integrity checking
void ibis::bitvector::read(const char * fn) {
    // let the file manager handle IO to avoid extra copying
    int ierr = ibis::fileManager::instance().getFile(fn, m_vec);
    if (ierr != 0) {
	if (ibis::gVerbose > 5)
	    ibis::util::logMessage("ibis::bitvector", "read(%s) is "
				   "unable to open the named file", fn);
	return;
    }

    if (m_vec.size() > 1) { // read a file correctly
	if (m_vec.back() > 0) { // has active bits
	    active.nbits = m_vec.back();
	    m_vec.pop_back();
	    active.val = m_vec.back();
	}
	else {
	    active.reset();
	}
	m_vec.pop_back();
    }

#if defined(WAH_CHECK_SIZE)
    nbits = do_cnt();
    // some integrity check here
    if (nbits % MAXBITS) {
	ibis::util::logger lg(-1);
	lg.buffer() << " Error *** ibis::bitvector::nbits(" << nbits
		    << ") is expected to be multiples of "
		    << MAXBITS << ", but it is not.";
	ierr ++;
    }
    if (nset > nbits+active.nbits) {
	ibis::util::logger lg(-1);
	lg.buffer() << " Error *** ibis::bitvector::nset (" << nset
		    << ") is expected to be not greater than "
		    << nbits+active.nbits << ", but it is.";
	ierr ++;
    }
    if (active.nbits >= MAXBITS) {
	ibis::util::logger lg(-1);
	lg.buffer() << " Error *** ibis::bitvector::active::nbits ("
		    << active.nbits << ") is expected to be less than "
		    << MAXBITS << ", but it is not.";
	ierr ++;
    }
#else
    nbits = 0;
    nset = 0;
#endif
#if defined(DEBUG) && DEBUG + 0 > 1
    if (nbits == 0 && ! m_vec.empty())
	nbits = do_cnt();
    if (size() > 0) {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "ibis::bitvector::read(" << fn << ") --\n";
	(void)print(lg.buffer());
    }
    else {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "empty file";
    }
#endif
    if (ierr) {
	ibis::util::logMessage("Error", "ibis::bitvector::read(%s) found "
			       "%d error%s in three sanity checks.",
			       fn, ierr, (ierr>1?"s":""));
	throw "bitvector::read failed integrity check";
    }
} // ibis::bitvector::read

// write bitvector to file (contents of vector is not changed)
void ibis::bitvector::write(const char * fn) const {
    FILE *out = fopen(fn, "wb");
    if (out == 0) {
	ibis::util::logMessage("Error", "ibis::bitvector::write() Failed to "
			       "open \"%s\" to write the bit vector ... %s",
			       fn, (errno ? strerror(errno) :
				    "no free stdio stream"));
	throw "bitvector::write failed to open file";
    }

#if defined(WAH_CHECK_SIZE)
    word_t nb = (nbits > 0 ? do_cnt() : nbits);
    if (nb != nbits) {
	ibis::util::logger lg(-1);
	print(lg.buffer());
	lg.buffer() << "Error ibis::bitvector::write() the value returned by "
		    << "do_cnt() (" << nb << ") is different from nbits ("
		    << nbits << ")\n" << "Reset nbits to " << nb;
    }
#endif
    word_t n = m_vec.size();
    word_t j = fwrite((const void*)m_vec.begin(), sizeof(word_t),
		      n, out);
    if (j != n) {
	ibis::util::logMessage("Error", "ibis::bitvector::write only "
			       "wrote %lu out of %lu words to %s",
			       static_cast<long unsigned>(j),
			       static_cast<long unsigned>(n), fn);
	fclose(out);
	throw "bitvector::write failed to write all bytes";
    }
#if defined(DEBUG) || defined(_DEBUG)
    active_word tmp(active);
    if (active.nbits >= MAXBITS) {
	tmp.nbits = active.nbits % MAXBITS;
	ibis::util::logMessage("Warning", "ibis::bitvector::write detects "
			       "larger than expected active.nbits (%lu, "
			       "MAX=%lu), setting it to %lu",
			       static_cast<long unsigned>(active.nbits),
			       static_cast<long unsigned>(MAXBITS),
			       static_cast<long unsigned>(tmp.nbits));
    }
    const word_t avmax = (1 << tmp.nbits) - 1;
    if (active.val > avmax) {
	tmp.val &= avmax;
	ibis::util::logMessage("Warning", "ibis::bitvector::write detects "
			       "larger than expected active.val (0x%.8x, "
			       "MAX=0x%.8x), setting it to 0x%.8x",
			       active.val, avmax, tmp.val);
    }
    if (tmp.nbits > 0) {
	fwrite((const void*)&(tmp.val), sizeof(word_t), 1, out);
    }
    fwrite((const void*)&(tmp.nbits), sizeof(word_t), 1, out);
#else
    if (active.nbits > 0) {
	fwrite((const void*)&(active.val), sizeof(word_t), 1, out);
    }
    fwrite((const void*)&(active.nbits), sizeof(word_t), 1, out);
#endif

    //     if (0 == fwrite((const void*)&nset, sizeof(word_t), 1, out)) {
    // 	ibis::util::logMessage("Error", "ibis::bitvector::write() fail to "
    // 			       "write nset to %s", fn);
    // 	fclose(out);
    // 	throw "bitvector::write failed to write the size";
    //     }

    //     if (0 == fwrite((const void*)&nbits, sizeof(word_t), 1, out)) {
    // 	ibis::util::logMessage("Error", "ibis::bitvector::write() fail to "
    // 			       "write nbits to %s", fn);
    // 	fclose(out);
    // 	throw "bitvector::write failed to write the cnt";
    //     }
    fflush(out);
    fclose(out);
} // ibis::bitvector::write

void ibis::bitvector::write(int out) const {
    if (out < 0)
	return;

#if defined(WAH_CHECK_SIZE)
    word_t nb = (nbits > 0 ? do_cnt() : nbits);
    if (nb != nbits) {
	ibis::util::logger lg(-1);
	lg.buffer() << "Error ibis::bitvector::write() the value returned by "
		    << "do_cnt() (" << nb << ") is different from nbits ("
		    << nbits << ")\n" << "Reset nbits to " << nb;
    }
#endif
    word_t n = sizeof(word_t) * m_vec.size();
    word_t j = UnixWrite(out, (const void*)m_vec.begin(), n);
    if (j != n) {
	ibis::util::logMessage("Error", "ibis::bitvector::write only "
			       "wrote %lu out of %lu bytes",
			       static_cast<long unsigned>(j),
			       static_cast<long unsigned>(n));
	throw "bitvector::write failed to write all bytes";
    }
#if defined(DEBUG) || defined(_DEBUG)
    active_word tmp(active);
    if (active.nbits >= MAXBITS) {
	tmp.nbits = active.nbits % MAXBITS;
	ibis::util::logMessage("Warning", "ibis::bitvector::write detects "
			       "larger than expected active.nbits (%lu, "
			       "MAX=%lu), setting it to %lu",
			       static_cast<long unsigned>(active.nbits),
			       static_cast<long unsigned>(MAXBITS),
			       static_cast<long unsigned>(tmp.nbits));
    }
    const word_t avmax = (1 << tmp.nbits) - 1;
    if (active.val > avmax) {
	tmp.val &= avmax;
	ibis::util::logMessage("Warning", "ibis::bitvector::write detects "
			       "larger than expected active.val (0x%.8x, "
			       "MAX=0x%.8x), setting it to 0x%.8x",
			       active.val, avmax, tmp.val);
    }
    if (tmp.nbits > 0) {
	UnixWrite(out, (const void*)&(tmp.val), sizeof(word_t));
    }
    UnixWrite(out, (const void*)&(tmp.nbits), sizeof(word_t));
#else
    if (active.nbits > 0) {
	UnixWrite(out, (const void*)&(active.val), sizeof(word_t));
    }
    UnixWrite(out, (const void*)&(active.nbits), sizeof(word_t));
    //     fwrite((const void*)&nset, sizeof(word_t), 1, out);
    //     fwrite((const void*)&nbits, sizeof(word_t), 1, out);
#endif
} // ibis::bitvector::write

void ibis::bitvector::write(array_t<ibis::bitvector::word_t>& arr) const {
    arr.reserve(m_vec.size()+2);
    arr.resize(m_vec.size());
    (void) memcpy(arr.begin(), m_vec.begin(), sizeof(word_t)*m_vec.size());
#if defined(DEBUG) || defined(_DEBUG)
    active_word tmp(active);
    if (active.nbits >= MAXBITS) {
	tmp.nbits = active.nbits % MAXBITS;
	ibis::util::logMessage("Warning", "ibis::bitvector::write detects "
			       "larger than expected active.nbits (%lu, "
			       "MAX=%lu), setting it to %lu",
			       static_cast<long unsigned>(active.nbits),
			       static_cast<long unsigned>(MAXBITS),
			       static_cast<long unsigned>(tmp.nbits));
    }
    const word_t avmax = (1 << tmp.nbits) - 1;
    if (active.val > avmax) {
	tmp.val &= avmax;
	ibis::util::logMessage("Warning", "ibis::bitvector::write detects "
			       "larger than expected active.val (0x%.8x, "
			       "MAX=0x%.8x), setting it to 0x%.8x",
			       active.val, avmax, tmp.val);
    }
    if (tmp.nbits > 0) {
	arr.push_back(tmp.val);
    }
    arr.push_back(tmp.nbits);
#else
    if (active.nbits > 0) {
	arr.push_back(active.val);
    }
    arr.push_back(active.nbits);
#endif
} // ibis::bitvector::write

/// Count the number of bits that are 1 in both this bitvector and mask.
/// This function assumes that both of them are compressed.
ibis::bitvector::word_t
ibis::bitvector::count_c2(const ibis::bitvector& mask) const {
    word_t cnt = cnt_ones(active.val & mask.active.val);
    run x, y;
    x.it = m_vec.begin();
    y.it = mask.m_vec.begin();
    while (x.it < m_vec.end() && y.it < mask.m_vec.end()) {
	if (x.nWords == 0)
	    x.decode();
	if (y.nWords == 0)
	    y.decode();

	if (x.isFill != 0) { // x is a fill
	    if (y.isFill != 0) { // y is a fill too
		if (x.fillBit != 0 && y.fillBit != 0) {
		    if (x.nWords <= y.nWords) {
			cnt += MAXBITS * x.nWords;
			y.nWords -= x.nWords;
			x.nWords = 0;
			++ x.it;
			y.it += (y.nWords == 0);
		    }
		    else {
			cnt += MAXBITS * y.nWords;
			x.nWords -= y.nWords;
			y.nWords = 0;
			++ y.it;
		    }
		}
		else if (x.nWords <= y.nWords) {
		    y.nWords -= x.nWords;
		    y.it += (y.nWords == 0);
		    x.nWords = 0;
		    ++ x.it;
		}
		else {
		    x.nWords -= y.nWords;
		    y.nWords = 0;
		    ++ y.it;
		}
	    }
	    else { // y is a literal word
		if (x.fillBit != 0)
		    cnt += cnt_ones(*y.it);
		++ y.it;
		-- x.nWords;
		y.nWords = 0;
		x.it += (x.nWords == 0);
	    }
	}
	else if (y.isFill != 0) { // x is a literal word, but y is a fill
	    if (y.fillBit != 0)
		cnt += cnt_ones(*x.it);
	    x.nWords = 0;
	    -- y.nWords;
	    ++ x.it;
	    y.it += (y.nWords == 0);
	}
	else { // both are literal words
	    cnt += cnt_ones(*x.it & *y.it);
	    x.nWords = 0;
	    y.nWords = 0;
	    ++ x.it;
	    ++ y.it;
	}
    }
    return cnt;
} // ibis::bitvector::count_c2

/// Count the number of bits that are 1 in both this bitvector and mask.
/// This function assumes mask is uncompressed and does not check this
/// fact.  The caller is responsible for ensuring this precondition is met.
ibis::bitvector::word_t
ibis::bitvector::count_c1(const ibis::bitvector& mask) const {
    run x;
    word_t cnt = cnt_ones(active.val & mask.active.val);
    x.it = m_vec.begin();
    array_t<word_t>::const_iterator it = mask.m_vec.begin();
    while (x.it < m_vec.end() && it < mask.m_vec.end()) {
	x.decode();
	if (x.isFill != 0) { // is a fill
	    if (x.fillBit != 0) {
		while (x.nWords > 0) {
		    cnt += cnt_ones(*it);
		    ++ it;
		    -- x.nWords;
		}
	    }
	    else {
		it += x.nWords;
	    }
	}
	else {
	    cnt += cnt_ones(*it & *x.it);
	    ++ it;
	}
	++ x.it;
    }
    return cnt;
} // ibis::bitvector::count_c1

// bitwise and (&) operation -- both operands may contain compressed words
void ibis::bitvector::and_c2(const ibis::bitvector& rhs,
			     ibis::bitvector& res) const {
    res.clear();
    if (m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.deepCopy(rhs.m_vec);
	    res.nbits = rhs.nbits;
	    res.nset = rhs.nset;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = 0;
	}
	else {
	    res.m_vec.push_back(*it & *(rhs.m_vec.begin()));
	    res.nbits = nbits;
	}
    }
    else if (rhs.m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = nset;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(rhs.m_vec);
	    res.nbits = rhs.nbits;
	    res.nset = 0;
	}
	else {
	    res.m_vec.push_back(*it & *(m_vec.begin()));
	    res.nbits = nbits;
	}
    }
    else if (m_vec.size() > 1) {
	run x, y;
	x.it = m_vec.begin();
	y.it = rhs.m_vec.begin();
	while (x.it < m_vec.end()) { // go through all words in m_vec
	    if (x.nWords == 0)
		x.decode();
	    if (y.nWords == 0)
		y.decode();
	    if (x.isFill != 0) {	    // x points to a fill
		// if both x and y point to fills, use the long one
		if (y.isFill != 0 && y.nWords >= x.nWords) {
		    if (y.fillBit == 0) {
			res.append_counter(0, y.nWords);
			x -= y.nWords;
			y.nWords = 0;
			++ y.it;
		    }
		    else {
			res.copy_runs(x, y.nWords);
			y.it += (y.nWords == 0);
		    }
		}
		else if (x.fillBit == 0) { // generate a 0-fill as the result
		    res.append_counter(0, x.nWords);
		    y -= x.nWords;
		    x.nWords = 0;
		    ++ x.it;
		}
		else { // copy the content of y
		    res.copy_runs(y, x.nWords);
		    x.it += (x.nWords == 0);
		}
	    }
	    else if (y.isFill != 0) {	    // i1 is compressed
		if (y.fillBit == 0) { // generate a 0-fill as the result
		    res.append_counter(0, y.nWords);
		    x -= y.nWords;
		    y.nWords = 0;
		    ++ y.it;
		}
		else { // copy the content of x
		    res.copy_runs(x, y.nWords);
		    y.it += (y.nWords == 0);
		}
	    }
	    else { // both words are not compressed
		res.active.val = *(x.it) & *(y.it);
		res.append_active();
		x.nWords = 0;
		y.nWords = 0;
		++ x.it;
		++ y.it;
	    }
	} // while (x.it < m_vec.end())

	if (x.it != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::and_c2 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left", static_cast<long>
				   (m_vec.end() - x.it));
	    throw "and_c2 iternal error";
	}

	if (y.it != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::and_c2 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left", static_cast<long>
				   (rhs.m_vec.end() - y.it));
	    throw "and_c2 iternal error";
	}
    }

    // the last thing -- work with the two active_words
    if (active.nbits) {
	res.active.val = active.val & rhs.active.val;
	res.active.nbits = active.nbits;
    }
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result of AND " << res;
#endif
} // bitvector& ibis::bitvector::and_c2

// and operation where rhs is not compressed (not one word is a counter)
void ibis::bitvector::and_c1(const ibis::bitvector& rhs,
			     ibis::bitvector& res) const {
    res.clear();
    if (m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.deepCopy(rhs.m_vec);
	    res.nbits = rhs.nbits;
	    res.nset = rhs.nset;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = 0;
	}
	else {
	    res.m_vec.push_back(*it & *(rhs.m_vec.begin()));
	    res.nbits = nbits;
	}
    }
    else if (m_vec.size() > 1) {
	array_t<word_t>::const_iterator i0 = m_vec.begin();
	array_t<word_t>::const_iterator i1 = rhs.m_vec.begin(), i2;
	word_t s0;
	res.m_vec.reserve(rhs.m_vec.size());
	while (i0 != m_vec.end()) {	// go through all words in m_vec
	    if (*i0 > ALLONES) { // i0 is compressed
		s0 = ((*i0) & MAXCNT);
		if ((*i0)<HEADER1) { // the result is all zero
		    if (s0 > 1)
			res.append_counter(0, s0);
		    else
			res.append_active();
		    i1 += s0;
		}
		else { // the result is *i1
		    i2 = i1 + s0;
		    for (; i1<i2; i1++)
			res.m_vec.push_back(*i1);
		    res.nbits += s0 * MAXBITS;
		}
	    }
	    else { // both words are not compressed
		res.active.val = *i0 & *i1;
		res.append_active();
		i1++;
	    }
	    i0++;
	} // while (i0 != m_vec.end())

	if (i1 != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::and_c1 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left", static_cast<long>
				   (rhs.m_vec.end() - i1));
	    throw "and_c1 iternal error";
	}
    }

    // the last thing -- work with the two active_words
    res.active.val = active.val & rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result of AND " << res;
#endif
} // ibis::bitvector::and_c1

// and operation on two compressed bitvectors, generates uncompressed result
void ibis::bitvector::and_d2(const ibis::bitvector& rhs,
			     ibis::bitvector& res) const {
    // establish a uncompressed bitvector with the right size
    res.nbits = ((nbits==0 && m_vec.size()>0) ?
		 (rhs.nbits == 0 && rhs.m_vec.size()>0) ? 
		 (m_vec.size() <= rhs.m_vec.size() ? do_cnt() : rhs.do_cnt())
		 : rhs.nbits : nbits);
    res.m_vec.resize(res.nbits/MAXBITS);

    // fill res with the right numbers
    if (m_vec.size() == 1) { // only one word in *this
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) { // copy rhs
	    rhs.decompress(res.m_vec);
	    res.nset = rhs.nset;
	}
	else if (*it > ALLONES) { // all zeros
	    decompress(res.m_vec);
	    res.nset = 0;
	}
	else {
	    res.m_vec[0] = (*it & *(rhs.m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (rhs.m_vec.size() == 1) { // only one word in rhs
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) { // copy *this
	    decompress(res.m_vec);
	    res.nset = nset;
	}
	else if (*it > ALLONES) { // all zero
	    rhs.decompress(res.m_vec);
	    res.nset = 0;
	}
	else {
	    res.m_vec[0] = (*it & *(m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (m_vec.size() > 1) { // more than one word in *this
	run x, y;
	res.nset = 0;
	x.it = m_vec.begin();
	y.it = rhs.m_vec.begin();
	array_t<word_t>::iterator ir = res.m_vec.begin();
	while (x.it < m_vec.end()) {	// go through all words in m_vec
	    if (x.nWords == 0)
		x.decode();
	    if (y.nWords == 0)
		y.decode();
	    if (x.isFill != 0) {
		if (y.isFill != 0 && y.nWords >= x.nWords) {
		    if (y.fillBit == 0) {
			x -= y.nWords;
			res.copy_fill(ir, y);
		    }
		    else {
			res.copy_runs(ir, x, y.nWords);
			y.it += (y.nWords == 0);
		    }
		}
		else if (x.fillBit == 0) {
		    y -= x.nWords;
		    res.copy_fill(ir, x);
		}
		else {
		    res.copy_runs(ir, y, x.nWords);
		    x.it += (x.nWords == 0);
		}
	    }
	    else if (y.isFill != 0) {
		if (y.fillBit == 0) {
		    x -= y.nWords;
		    res.copy_fill(ir, y);
		}
		else {
		    res.copy_runs(ir, x, y.nWords);
		    y.it += (y.nWords == 0);
		}
	    }
	    else { // both words are not compressed
		*ir = *x.it & *y.it;
		x.nWords = 0;
		y.nWords = 0;
		++ x.it;
		++ y.it;
		++ ir;
	    }
	} // while (x.it < m_vec.end())

	if (x.it != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::and_d2 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left", static_cast<long>
				   (m_vec.end() - x.it));
	    throw "and_d2 internal error";
	}

	if (y.it != rhs.m_vec.end()) {
	    word_t nb0 = do_cnt() + active.nbits;
	    word_t nb1 = rhs.do_cnt() + rhs.active.nbits;
	    ibis::util::logMessage("Error", "ibis::bitvector::and_d2 "
				   "between two bit vectors of sizes "
				   "%lu:%lu and %lu:%lu "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left",
				   static_cast<long unsigned>(nb0),
				   static_cast<long unsigned>(bytes()),
				   static_cast<long unsigned>(nb1),
				   static_cast<long unsigned>(rhs.bytes()),
				   static_cast<long int>
				   (rhs.m_vec.end()-y.it));
	    throw "and_d2 internal error";
	}

	if (ir != res.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::and_d2 "
				   "expects to exhaust ir but there are %ld "
				   "word(s) left", static_cast<long>
				   (res.m_vec.end() - ir));
	    throw "and_d2 internal error";
	}
    }

    // work with the two active_words
    res.active.val = active.val & rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "operand 1 of AND" << *this << std::endl;
    lg.buffer() << "operand 2 of AND" << rhs << std::endl;
    lg.buffer() << "result of AND " << res;
#endif
} // ibis::bitvector::and_d2

// assuming *this is uncompressed but rhs is not, this function performs the
// AND operation and overwrites *this with the result
void ibis::bitvector::and_d1(const ibis::bitvector& rhs) {
    m_vec.nosharing(); // make sure *this is not shared!
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operand 1 of AND" << *this << std::endl;
	lg.buffer() << "operand 2 of AND" << rhs << std::endl;
    }
#endif
    if (rhs.m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it < HEADER1) { // nothing to do for *it >= HEADER1
	    if (*it > ALLONES) { // all zero
		memset(m_vec.begin(), 0, sizeof(word_t)*m_vec.size());
		nset = 0;
	    }
	    else { // one literal word
		m_vec[0] = (*it & *(rhs.m_vec.begin()));
		nset = cnt_ones(m_vec[0]);
	    }
	}
    }
    else if (rhs.m_vec.size() > 1) {
	array_t<word_t>::iterator i0 = m_vec.begin();
	array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
	word_t s0;
	nset = 0;
	while (i1 != rhs.m_vec.end()) {	// go through all words in m_vec
	    if (*i1 > ALLONES) { // i1 is compressed
		s0 = ((*i1) & MAXCNT);
		if ((*i1) < HEADER1) { // set literal words to zero
		    memset(i0, 0, sizeof(word_t)*s0);
		}
		i0 += s0;
	    }
	    else { // both words are not compressed
		*i0 &= *i1;
		++ i0;
	    }
	    ++ i1;
	} // while (i1 != rhs.m_vec.end())

	if (i0 != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::and_d1 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left", static_cast<long>
				   (m_vec.end() - i0));
	    throw "and_d1 internal error";
	}
    }

    // the last thing -- work with the two active_words
    active.val &= rhs.active.val;
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "result of AND" << *this;
    }
#endif
} // ibis::bitvector::and_d1

// both operands of the 'and' operation are not compressed
void ibis::bitvector::and_c0(const ibis::bitvector& rhs) {
    nset = 0;
    m_vec.nosharing(); // make sure *this is not shared!
    array_t<word_t>::iterator i0 = m_vec.begin();
    array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
    while (i0 != m_vec.end()) {	// go through all words in m_vec
	*i0 &= *i1;
	i0++; i1++;
    } // while (i0 != m_vec.end())

    // the last thing -- work with the two active_words
    active.val &= rhs.active.val;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result of AND " << *this;
#endif
} // ibis::bitvector::and_c0

// or operation on two compressed bitvectors
void ibis::bitvector::or_c2(const ibis::bitvector& rhs,
			    ibis::bitvector& res) const {
    res.clear();
    if (m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = nbits;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(rhs.m_vec);
	    res.nbits = rhs.nbits;
	    res.nset = rhs.nset;
	}
	else {
	    res.m_vec.push_back(*it | *(rhs.m_vec.begin()));
	    res.nbits = nbits;
	}
    }
    else if (rhs.m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.deepCopy(rhs.m_vec);
	    res.nbits = rhs.nbits;
	    res.nset = rhs.nbits;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = nset;
	}
	else {
	    res.m_vec.push_back(*it | *(m_vec.begin()));
	    res.nbits = nbits;
	}
    }
    else if (m_vec.size() > 1) {
	run x, y;
	x.it = m_vec.begin();
	y.it = rhs.m_vec.begin();
	while (x.it < m_vec.end()) {	// go through all words in m_vec
	    if (x.nWords == 0)
		x.decode();
	    if (y.nWords == 0)
		y.decode();
	    if (x.nWords == 0 || y.nWords == 0) {
		LOGGER(0) << " Error *** ibis::bitvector::or_c2 serious "
		    "problem here ...";
	    }
	    if (x.isFill != 0) { // x points to a fill
		// if both x and y point to fills, use the longer one
		if (y.isFill != 0 && y.nWords >= x.nWords) {
		    if (y.fillBit) {
			res.append_counter(y.fillBit, y.nWords);
			x -= y.nWords;
			y.nWords = 0;
			++ y.it;
		    }
		    else {
			res.copy_runs(x, y.nWords);
			y.it += (y.nWords == 0);
		    }
		}
		else if (x.fillBit) { // the result is all ones
		    res.append_counter(x.fillBit, x.nWords);
		    y -= x.nWords; // advance the pointer in y
		    x.nWords = 0;
		    ++ x.it;
		}
		else { // copy the content of y
		    res.copy_runs(y, x.nWords);
		    x.it += (x.nWords == 0);
		}
	    }
	    else if (y.isFill != 0) { // y points to a fill
		if (y.fillBit) {
		    res.append_counter(y.fillBit, y.nWords);
		    x -= y.nWords;
		    y.nWords = 0;
		    ++ y.it;
		}
		else {
		    res.copy_runs(x, y.nWords);
		    y.it += (y.nWords == 0);
		}
	    }
	    else { // both words are not compressed
		res.active.val = *x.it | *y.it;
		res.append_active();
		x.nWords = 0;
		y.nWords = 0;
		++ x.it;
		++ y.it;
	    }
	} // while (x.it < m_vec.end())

	if (x.it != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::or_c2 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left", static_cast<long>
				   (m_vec.end() - x.it));
#if defined(DEBUG) && DEBUG + 0 > 1
	    {
		ibis::util::logger lg(ibis::gVerbose);
		lg.buffer() << "*this: " << *this << std::endl;
		lg.buffer() << "rhs: " << rhs << std::endl;
		lg.buffer() << "res: " << res;
	    }
#else
	    throw "or_c2 internal error";
#endif
	}

	if (y.it != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::or_c2 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left",
				   static_cast<long>(rhs.m_vec.end() - y.it));
#if defined(DEBUG) && DEBUG + 0 > 1
	    {
		ibis::util::logger lg(ibis::gVerbose);
		lg.buffer() << "*this: " << *this << std::endl;
		lg.buffer() << "rhs: " << rhs << std::endl;
		lg.buffer() << "res: " << res;
	    }
#else
	    throw "or_c2 internal error";
#endif
	}
    }

    // work with the two active_words
    res.active.val = active.val | rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "operand 1 of OR" << *this << std::endl;
    lg.buffer() << "operand 2 of OR" << rhs << std::endl;
    lg.buffer() << "result of OR " << res;
#endif
} // ibis::bitvector::or_c2

// or operation where rhs is not compressed (not one word is a counter)
void ibis::bitvector::or_c1(const ibis::bitvector& rhs,
			    ibis::bitvector& res) const {
    res.clear();
    if (m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = nbits;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(rhs.m_vec);
	    res.nbits = rhs.nbits;
	    res.nset = rhs.nset;
	}
	else {
	    res.m_vec.push_back(*it | *(rhs.m_vec.begin()));
	    res.nbits = nbits;
	}
    }
    else if (m_vec.size() > 1) {
	array_t<word_t>::const_iterator i0 = m_vec.begin();
	array_t<word_t>::const_iterator i1 = rhs.m_vec.begin(), i2;
	word_t s0;
	res.m_vec.reserve(rhs.m_vec.size());
	while (i0 != m_vec.end()) {	// go through all words in m_vec
	    if (*i0 > ALLONES) { // i0 is compressed
		s0 = ((*i0) & MAXCNT);
		if ((*i0)>=HEADER1) { // the result is all ones
		    if (s0 > 1)
			res.append_counter(1, s0);
		    else {
			res.active.val = ALLONES;
			res.append_active();
		    }
		    i1 += s0;
		}
		else { // the result is *i1
		    i2 = i1 + s0;
		    for (; i1<i2; i1++)
			res.m_vec.push_back(*i1);
		    res.nbits += s0 * MAXBITS;
		}
	    }
	    else { // both words are not compressed
		res.active.val = *i0 | *i1;
		res.append_active();
		i1++;
	    }
	    i0++;
	} // while (i0 != m_vec.end())

	if (i1 != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::or_c1 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left",
				   static_cast<long>(rhs.m_vec.end() - i1));
	    throw "or_c1 internal error";
	}
    }

    // the last thing -- work with the two active_words
    res.active.val = active.val | rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "operand 1 of OR" << *this << std::endl;
    lg.buffer() << "operand 2 of OR" << rhs << std::endl;
    lg.buffer() << "result or OR " << res;
#endif
} // ibis::bitvector::or_c1

// or operation on two compressed bitvectors, generates uncompressed result
void ibis::bitvector::or_d2(const ibis::bitvector& rhs,
			    ibis::bitvector& res) const {
    // establish a uncompressed bitvector with the right size
    res.nbits = ((nbits==0 && m_vec.size()>0) ?
		 (rhs.nbits == 0 && rhs.m_vec.size()>0) ? 
		 (m_vec.size() <= rhs.m_vec.size() ? do_cnt() : rhs.do_cnt())
		 : rhs.nbits : nbits);
    res.m_vec.resize(res.nbits/MAXBITS);

    // fill res with the right numbers
    if (m_vec.size() == 1) { // only one word in *this
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) { // all ones
	    decompress(res.m_vec);
	    res.nset = nbits;
	}
	else if (*it > ALLONES) { // copy rhs
	    rhs.decompress(res.m_vec);
	    res.nset = rhs.nset;
	}
	else {
	    res.m_vec[0] = (*it | *(rhs.m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (rhs.m_vec.size() == 1) { // only one word in rhs
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) { // all ones
	    rhs.decompress(res.m_vec);
	    res.nset = rhs.nbits;
	}
	else if (*it > ALLONES) { // copy *this
	    decompress(res.m_vec);
	    res.nset = nset;
	}
	else {
	    res.m_vec[0] = (*it | *(m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (m_vec.size() > 1) { // more than one word in *this
	run x, y;
	res.nset = 0;
	x.it = m_vec.begin();
	y.it = rhs.m_vec.begin();
	array_t<word_t>::iterator ir = res.m_vec.begin();
	while (x.it < m_vec.end()) {	// go through all words in m_vec
	    if (x.nWords == 0)
		x.decode();
	    if (y.nWords == 0)
		y.decode();
	    if (x.isFill != 0) {
		if (y.isFill != 0 && y.nWords >= x.nWords) {
		    if (y.fillBit == 0) {
			res.copy_runs(ir, x, y.nWords);
			y.it += (y.nWords == 0);
		    }
		    else {
			x -= y.nWords;
			res.copy_fill(ir, y);
		    }
		}
		else if (x.fillBit == 0) {
		    res.copy_runs(ir, y, x.nWords);
		    x.it += (x.nWords == 0);
		}
		else {
		    y -= x.nWords;
		    res.copy_fill(ir, x);
		}
	    }
	    else if (y.isFill != 0) {
		if (y.fillBit == 0) {
		    res.copy_runs(ir, x, y.nWords);
		    y.it += (y.nWords == 0);
		}
		else {
		    x -= y.nWords;
		    res.copy_fill(ir, y);
		}
	    }
	    else { // both words are not compressed
		*ir = *x.it | *y.it;
		x.nWords = 0;
		y.nWords = 0;
		++ x.it;
		++ y.it;
		++ ir;
	    }
	} // while (x.it < m_vec.end())

	if (x.it != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::or_d2 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left",
				   static_cast<long>(m_vec.end() - x.it));
	    LOGGER(0) << "Operand 1 or or_d2 " << *this
		      << "\nOperand 2 or or_d2 " << rhs
		      << "\nResult of or_d2 " << res;
	    throw "or_d2 internal error";
	}

	if (y.it != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::or_d2 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left",
				   static_cast<long>(rhs.m_vec.end() - y.it));
	    LOGGER(0) << "Operand 1 or or_d2 " << *this
		      << "\nOperand 2 or or_d2 " << rhs
		      << "\nResult of or_d2 " << res;
	    throw "or_d2 internal error";
	}

	if (ir != res.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::or_d2 "
				   "expects to exhaust ir but there are %ld "
				   "word(s) left",
				   static_cast<long>(res.m_vec.end() - ir));
	    LOGGER(0) << "Operand 1 or or_d2 " << *this
		      << "\nOperand 2 or or_d2 " << rhs
		      << "\nResult of or_d2 " << res;
	    throw "or_d2 internal error";
	}
    }

    // work with the two active_words
    res.active.val = active.val | rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    LOGGER(0)
	<< "Operand 1 or or_d2 " << *this
	<< "\nOperand 2 or or_d2 " << rhs
	<< "\nResult of or_d2 " << res;
#endif
} // ibis::bitvector::or_d2

// assuming *this is uncompressed but rhs is not, this function performs
// the OR operation and overwrites *this with the result
void ibis::bitvector::or_d1(const ibis::bitvector& rhs) {
    m_vec.nosharing(); // make sure *this is not shared!
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operand 1 of OR" << *this << std::endl;
	lg.buffer() << "operand 2 of OR" << rhs;
    }
#endif
    if (rhs.m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) { // all ones
	    rhs.decompress(m_vec);
	    nset = nbits;
	}
	else if (*it <= ALLONES) { // one literal word
	    m_vec[0] = (*it | *(rhs.m_vec.begin()));
	    nset = cnt_ones(m_vec[0]);
	}
    }
    else if (rhs.m_vec.size() > 1) {
	array_t<word_t>::iterator i0 = m_vec.begin();
	array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
	word_t s0;
	nset = 0;
	while (i1 != rhs.m_vec.end()) {	// go through all words in m_vec
	    if (*i1 > ALLONES) { // i1 is compressed
		s0 = ((*i1) & MAXCNT);
		if ((*i1) >= HEADER1) { // the result is all ones
		    array_t<word_t>::const_iterator stp = i0 + s0;
		    while (i0 < stp) {
			*i0 = ALLONES;
			++ i0;
		    }
		}
		else { // the result is *i0
		    i0 += s0;
		}
	    }
	    else { // both words are not compressed
		*i0 |= *i1;
		++ i0;
	    }
	    ++ i1;
	} // while (i1 != rhs.m_vec.end())

	if (i0 != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::or_d1 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left",
				   static_cast<long>(m_vec.end() - i0));
	    throw "or_d1 internal error";
	}
    }

    // the last thing -- work with the two active_words
    active.val |= rhs.active.val;
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "result of OR" << *this;
    }
#endif
} // ibis::bitvector::or_d1

// both operands of the 'or' operation are not compressed
void ibis::bitvector::or_c0(const ibis::bitvector& rhs) {
    m_vec.nosharing(); // make sure *this is not shared!
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operand 1 of OR" << *this << std::endl;
	lg.buffer() << "operand 2 of OR" << rhs;
    }
#endif
    nset = 0;
    array_t<word_t>::iterator i0 = m_vec.begin();
    array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
    while (i0 != m_vec.end()) {	// go through all words in m_vec
	*i0 |= *i1;
	i0++; i1++;
    } // while (i0 != m_vec.end())

    // the last thing -- work with the two active_words
    active.val |= rhs.active.val;
#if defined(DEBUG) && DEBUG + 0 > 1
    word_t nb = do_cnt();
    if (nbits == 0)
	nbits = nb;
    if (nb != rhs.nbits && rhs.nbits > 0) {
	ibis::util::logMessage("Warning", "ibis::bitvector::or_c0=() expects "
			       "to have %lu bits but got %lu",
			       static_cast<long unsigned>(rhs.nbits),
			       static_cast<long unsigned>(nb));
    }
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "result of OR " << *this;
    }
    //      if (m_vec.size()*(MAXBITS<<1) >= nbits)
    //  	decompress();
#endif
} // ibis::bitvector::or_c0

// bitwise xor (^) operation -- both operands may contain compressed words
void ibis::bitvector::xor_c2(const ibis::bitvector& rhs,
			     ibis::bitvector& res) const {
    run x, y;
    res.clear();
    x.it = m_vec.begin();
    y.it = rhs.m_vec.begin();
    while (x.it < m_vec.end()) {	// go through all words in m_vec
	if (x.nWords == 0)
	    x.decode();
	if (y.nWords == 0)
	    y.decode();
	if (x.isFill != 0) { // x points to a fill
	    // if both x and y point to a fill, use the longer fill
	    if (y.isFill != 0 && y.nWords >= x.nWords) {
		if (y.fillBit == 0)
		    res.copy_runs(x, y.nWords);
		else
		    res.copy_runsn(x, y.nWords);
		y.it += (y.nWords == 0);
	    }
	    else if (x.fillBit == 0) {
		res.copy_runs(y, x.nWords);
		x.it += (x.nWords == 0);
	    }
	    else {
		res.copy_runsn(y, x.nWords);
		x.it += (x.nWords == 0);
	    }
	}
	else if (y.isFill != 0) {	// y points to a fill
	    if (y.fillBit == 0)
		res.copy_runs(x, y.nWords);
	    else
		res.copy_runsn(x, y.nWords);
	    y.it += (y.nWords == 0);
	}
	else { // both words are not compressed
	    res.active.val = *x.it ^ *y.it;
	    res.append_active();
	    x.nWords = 0;
	    y.nWords = 0;
	    ++ x.it;
	    ++ y.it;
	}
    } // while (x.it < m_vec.end())

    if (x.it != m_vec.end()) {
	ibis::util::logMessage("Error", "ibis::bitvector::xor_c2 expects "
			       "to exhaust i0 but there are %ld word(s) left",
			       static_cast<long int>(m_vec.end() - x.it));
	throw "xor_c2 interal error";
    }

    if (y.it != rhs.m_vec.end()) {
	{
	    ibis::util::logger lg(-1);
	    lg.buffer() << "Bitvector 1\n" << *this << "\n Bitvector 2\n"
			<< rhs << "\nXOR result so far\n" << res;
	}
	ibis::util::logMessage("Error", "ibis::bitvector::xor_c2 expects "
			       "to exhaust i1 but there are %ld word(s) left",
			       static_cast<long int>(rhs.m_vec.end() - y.it));
	throw "xor_c2 internal error";
    }

    // the last thing -- work with the two active_words
    res.active.val = active.val ^ rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result of XOR " << res << std::endl;
#endif
} // bitvector& ibis::bitvector::xor_c2

// xor operation where rhs is not compressed (not one word is a counter)
void ibis::bitvector::xor_c1(const ibis::bitvector& rhs,
			     ibis::bitvector& res) const {
    array_t<word_t>::const_iterator i0 = m_vec.begin();
    array_t<word_t>::const_iterator i1 = rhs.m_vec.begin(), i2;
    word_t s0;
    res.clear();
    res.m_vec.reserve(rhs.m_vec.size());
    while (i0 != m_vec.end()) {	// go through all words in m_vec
	if (*i0 > ALLONES) { // i0 is compressed
	    s0 = ((*i0) & MAXCNT);
	    i2 = i1 + s0;
	    res.nbits += s0*MAXBITS;
	    if ((*i0)>=HEADER1) { // the result is the compliment of i1
		for (; i1!=i2; i1++)
		    res.m_vec.push_back((*i1) ^ ALLONES);
	    }
	    else { // the result is *i1
		for (; i1!=i2; i1++)
		    res.m_vec.push_back(*i1);
	    }
	}
	else { // both words are not compressed
	    res.active.val = *i0 ^ *i1;
	    res.append_active();
	    i1++;
	}
	i0++;
    } // while (i0 != m_vec.end())

    if (i1 != rhs.m_vec.end()) {
	ibis::util::logMessage("Error", "ibis::bitvector::xor_c1 expects "
			       "to exhaust i1 but there are %ld word(s) left",
			       static_cast<long int>(rhs.m_vec.end() - i1));
	throw "xor_c1 iternal error";
    }

    // the last thing -- work with the two active_words
    res.active.val = active.val ^ rhs.active.val;
    res.active.nbits = active.nbits;

#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result XOR " << res;
#endif
} // ibis::bitvector::xor_c1

// xor operation on two compressed bitvectors, generates uncompressed result
void ibis::bitvector::xor_d2(const ibis::bitvector& rhs,
			     ibis::bitvector& res) const {
    // establish a uncompressed bitvector with the right size
    res.nbits = ((nbits==0 && m_vec.size()>0) ?
		 (rhs.nbits == 0 && rhs.m_vec.size()>0) ? 
		 (m_vec.size() <= rhs.m_vec.size() ? do_cnt() : rhs.do_cnt())
		 : rhs.nbits : nbits);
    res.m_vec.resize(res.nbits/MAXBITS);

    // fill res with the right numbers
    if (m_vec.size() == 1) { // only one word in *this
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) { // complement of rhs
	    rhs.copy_comp(res.m_vec);
	    if (rhs.nset > 0)
		res.nset = nbits - rhs.nset;
	}
	else if (*it > ALLONES) { // copy rhs
	    rhs.decompress(res.m_vec);
	    res.nset = rhs.nset;
	}
	else {
	    res.m_vec[0] = (*it ^ *(rhs.m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (rhs.m_vec.size() == 1) { // only one word in rhs
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) { // complement of *this
	    copy_comp(res.m_vec);
	    if (nset > 0) res.nset = nbits - nset;
	}
	else if (*it > ALLONES) { // copy *this
	    decompress(res.m_vec);
	    res.nset = nset;
	}
	else {
	    res.m_vec[0] = (*it ^ *(m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (m_vec.size() > 1) { // more than one word in *this
	run x, y;
	res.nset = 0;
	x.it = m_vec.begin();
	y.it = rhs.m_vec.begin();
	array_t<word_t>::iterator ir = res.m_vec.begin();
	while (x.it < m_vec.end()) {	// go through all words in m_vec
	    if (x.nWords == 0)
		x.decode();
	    if (y.nWords == 0)
		y.decode();
	    if (x.isFill != 0) {
		if (y.isFill != 0 && y.nWords >= x.nWords) {
		    if (y.fillBit == 0) {
			res.copy_runs(ir, x, y.nWords);
		    }
		    else {
			res.copy_runsn(ir, x, y.nWords);
		    }
		    y.it += (y.nWords == 0);
		}
		else {
		    if (x.fillBit == 0) {
			res.copy_runs(ir, y, x.nWords);
		    }
		    else {
			res.copy_runsn(ir, y, x.nWords);
		    }
		    x.it += (x.nWords == 0);
		}
	    }
	    else if (y.isFill != 0) {
		if (y.fillBit == 0) {
		    res.copy_runs(ir, x, y.nWords);
		}
		else {
		    res.copy_runsn(ir, x, y.nWords);
		}
		y.it += (y.nWords == 0);
	    }
	    else { // both words are not compressed
		*ir = *x.it ^ *y.it;
		x.nWords = 0;
		y.nWords = 0;
		++ x.it;
		++ y.it;
		++ ir;
	    }
	} // while (x.it < m_vec.end())

	if (x.it != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::xor_d2 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left",
				   static_cast<long int>(m_vec.end() - x.it));
	    throw "xor_d2 internal error";
	}

	if (y.it != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::xor_d2 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left", static_cast<long int>
				   (rhs.m_vec.end() - y.it));
	    throw "xor_d2 internal error";
	}

	if (ir != res.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::xor_d2 "
				   "expects to exhaust ir but there are %ld "
				   "word(s) left", static_cast<long int>
				   (res.m_vec.end() - ir));
	    throw "xor_d2 internal error";
	}
    }

    // work with the two active_words
    res.active.val = active.val ^ rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "operand 1 of XOR" << *this << std::endl;
    lg.buffer() << "operand 2 of XOR" << rhs << std::endl;
    lg.buffer() << "result of XOR " << res;
#endif
} // ibis::bitvector::xor_d2

// assuming *this is uncompressed but rhs is compressed, this function
// performs the XOR operation and overwrites *this with the result
void ibis::bitvector::xor_d1(const ibis::bitvector& rhs) {
    m_vec.nosharing(); // make sure *this is not shared!
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operand 1 of XOR" << *this << std::endl;
	lg.buffer() << "operand 2 of XOR" << rhs;
    }
#endif
    if (rhs.m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) { // complement every bit
	    flip();
	}
	else if (*it <= ALLONES) {
	    m_vec[0] = (*it ^ *(rhs.m_vec.begin()));
	    nset = cnt_ones(m_vec[0]);
	}
    }
    else if (rhs.m_vec.size() > 1) {
	nset = 0;
	word_t s0;
	array_t<word_t>::iterator i0 = m_vec.begin();
	array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
	while (i1 != rhs.m_vec.end()) {	// go through all words in m_vec
	    if (*i1 > ALLONES) { // i1 is compressed
		s0 = ((*i1) & MAXCNT);
		if ((*i1) >= HEADER1) { // the result is the complement of i0
		    array_t<word_t>::const_iterator stp = i0 + s0;
		    while (i0 < stp) {
			*i0 ^= ALLONES;
			++ i0;
		    }
		}
		else { // the result is *i0
		    i0 += s0;
		}
	    }
	    else { // both words are not compressed
		*i0 ^= *i1;
		++ i0;
	    }
	    ++ i1;
	} // while (i1 != rhs.m_vec.end())

	if (i0 != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::xor_d1 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left", static_cast<long int>
				   (m_vec.end() - i0));
	    throw "xor_d1 internal error";
	}
    }

    // the last thing -- work with the two active_words
    active.val ^= rhs.active.val;
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "result of XOR" << *this;
    }
#endif
} // ibis::bitvector::xor_d1

// both operands of the 'xor' operation are not compressed
void ibis::bitvector::xor_c0(const ibis::bitvector& rhs) {
    m_vec.nosharing(); // make sure *this is not shared!
    nset = 0;
    array_t<word_t>::iterator i0 = m_vec.begin();
    array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
    while (i0 != m_vec.end()) {	// go through all words in m_vec
	*i0 ^= *i1;
	i0++; i1++;
    } // while (i0 != m_vec.end())

    // the last thing -- work with the two active_words
    active.val ^= rhs.active.val;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result XOR " << *this << std::endl;
    /*
      if (m_vec.size()*(MAXBITS<<1) >= nbits)
      decompress();
    */
#endif
} // ibis::bitvector::xor_c0

// bitwise minus (&!) operation -- both operands may contain compressed words
void ibis::bitvector::minus_c2(const ibis::bitvector& rhs,
			       ibis::bitvector& res) const {
    res.clear();
    if (m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.resize(rhs.m_vec.size());
	    for (word_t i=0; i<rhs.m_vec.size(); ++i) {
		if (rhs.m_vec[i] > ALLONES)
		    res.m_vec[i] = (rhs.m_vec[i] ^ FILLBIT);
		else
		    res.m_vec[i] = (rhs.m_vec[i] ^ ALLONES);
	    }
	    res.nbits = rhs.nbits;
	    if (rhs.nset > 0)
		res.nset = rhs.nbits - rhs.nset;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = 0;
	}
	else {
	    res.m_vec.push_back(*it & ~(*(rhs.m_vec.begin())));
	    res.nbits = nbits;
	}
    }
    else if (rhs.m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) {
	    res.appendFill(0, rhs.nbits);
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = nset;
	}
	else {
	    res.m_vec.push_back(*it & *(m_vec.begin()));
	    res.nbits = nbits;
	}
    }
    else if (m_vec.size() > 1) {
	run x, y;
	x.it = m_vec.begin();
	y.it = rhs.m_vec.begin();
	while (x.it < m_vec.end()) {	// go through all words in m_vec
	    if (x.nWords == 0)
		x.decode();
	    if (y.nWords == 0)
		y.decode();
	    if (x.isFill != 0) {
		if (y.isFill != 0 && y.nWords >= x.nWords) {
		    if (y.fillBit == 0) {
			res.copy_runs(x, y.nWords);
			y.it += (y.nWords == 0);
		    }
		    else {
			res.append_counter(0, y.nWords);
			x -= y.nWords;
			y.nWords = 0;
			++ y.it;
		    }
		}
		else if (x.fillBit == 0) {
		    res.append_counter(0, x.nWords);
		    y -= x.nWords;
		    x.nWords = 0;
		    ++ x.it;
		}
		else {
		    res.copy_runsn(y, x.nWords);
		    x.it += (x.nWords == 0);
		}
	    }
	    else if (y.isFill != 0) {	    // y is compressed but not x
		if (y.fillBit == 0) {
		    res.copy_runs(x, y.nWords);
		    y.it += (y.nWords == 0);
		}
		else {
		    res.append_counter(0, y.nWords);
		    x -= y.nWords;
		    y.nWords = 0;
		    ++ y.it;
		}
	    }
	    else { // both words are not compressed
		res.active.val = *x.it & ~(*y.it);
		res.append_active();
		x.nWords = 0;
		y.nWords = 0;
		++ x.it;
		++ y.it;
	    }
	} // while (x.it < m_vec.end())

	if (x.it != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::minus_c2 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left", static_cast<long int>
				   (m_vec.end() - x.it));
	    throw "minus_c2 internal error";
	}

	if (y.it != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::minus_c2 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left", static_cast<long int>
				   (rhs.m_vec.end() - y.it));
	    throw "minus_c2 internal error";
	}
    }

    // the last thing -- work with the two active_words
    res.active.val = active.val & ~(rhs.active.val);
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result MINUS " << res;
    /*
      if (res.m_vec.size()*(MAXBITS<<1) >= res.nbits)
      res.decompress();
    */
#endif
} // bitvector& ibis::bitvector::minus_c2

// minus operation where rhs is not compressed (not one word is a counter)
void ibis::bitvector::minus_c1(const ibis::bitvector& rhs,
			       ibis::bitvector& res) const {
    res.clear();
    if (m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) {
	    res.m_vec.resize(rhs.m_vec.size());
	    for (word_t i=0; i<rhs.m_vec.size(); ++i)
		res.m_vec[i] = (rhs.m_vec[i] ^ ALLONES);
	    res.nbits = rhs.nbits;
	    if (rhs.nset > 0)
		res.nset = rhs.nbits - rhs.nset;
	}
	else if (*it > ALLONES) {
	    res.m_vec.deepCopy(m_vec);
	    res.nbits = nbits;
	    res.nset = 0;
	}
	else {
	    res.m_vec.push_back(*it & ~(*(rhs.m_vec.begin())));
	    res.nbits = nbits;
	}
    }
    else if (m_vec.size() > 1) {
	array_t<word_t>::const_iterator i0 = m_vec.begin();
	array_t<word_t>::const_iterator i1 = rhs.m_vec.begin(), i2;
	word_t s0;
	res.clear();
	res.m_vec.reserve(rhs.m_vec.size());
	while (i0 != m_vec.end()) {	// go through all words in m_vec
	    if (*i0 > ALLONES) { // i0 is compressed
		s0 = ((*i0) & MAXCNT);
		i2 = i1 + s0;
		if ((*i0)>=HEADER1) { // the result is the compliment of i1
		    for (; i1 < i2; ++i1)
			res.m_vec.push_back((*i1) ^ ALLONES);
		    res.nbits += s0*MAXBITS;
		}
		else { // the result is a fill of zero bits
		    i1 = i2;
		    if (s0 > 1)
			res.append_counter(0, s0);
		    else
			res.append_active();
		}
	    }
	    else { // both words are not compressed
		res.active.val = *i0 & ~(*i1);
		res.append_active();
		i1++;
	    }
	    i0++;
	} // while (i0 != m_vec.end())

	if (i1 != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::minus_c1 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left", static_cast<long int>
				   (rhs.m_vec.end() - i1));
	    throw "minus_c1 internal error";
	}
    }

    // the last thing -- work with the two active_words
    res.active.val = active.val & ~(rhs.active.val);
    res.active.nbits = active.nbits;

#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result MINUS " << res;
#endif
} // ibis::bitvector::minus_c1

// minus operation where *this is not compressed (not one word is a counter)
void ibis::bitvector::minus_c1x(const ibis::bitvector& rhs,
				ibis::bitvector& res) const {
    array_t<word_t>::const_iterator i0 = m_vec.begin();
    array_t<word_t>::const_iterator i1 = rhs.m_vec.begin(), i2;
    word_t s0;
    res.clear();
    res.m_vec.reserve(rhs.m_vec.size());
    while (i1 != rhs.m_vec.end()) {	// go through all words in m_vec
	if (*i1 > ALLONES) { // i1 is compressed
	    s0 = ((*i1) & MAXCNT);
	    i2 = i0 + s0;
	    if ((*i1) >= HEADER1) {	// the result is a fill of zero bits
		i0 = i2;
		if (s0 > 1)
		    res.append_counter(0, s0);
		else
		    res.append_active();
	    }
	    else {	// the result is i0
		for (; i0 < i2; ++i0)
		    res.m_vec.push_back(*i0);
		res.nbits += s0*MAXBITS;
	    }
	}
	else {	// both words are not compressed
	    res.active.val = *i0 & ~(*i1);
	    res.append_active();
	    i0++;
	}
	i1++;
    } // while (i1 != rhs.m_vec.end())

    if (i0 != m_vec.end()) {
	ibis::util::logMessage("Error", "ibis::bitvector::minus_c1x expects "
			       "to exhaust i0 but there are %ld word(s) left",
			       static_cast<long int>(m_vec.end() - i0));
	throw "minus_c1x internal error";
    }

    // the last thing -- work with the two active_words
    res.active.val = active.val & ~(rhs.active.val);
    res.active.nbits = active.nbits;

#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result MINUS " << res;
#endif
} // ibis::bitvector::minus_c1x

// minus operation on two compressed bitvectors, it differs from minus_c2 in
// that this one generates a uncompressed result
void ibis::bitvector::minus_d2(const ibis::bitvector& rhs,
			       ibis::bitvector& res) const {
    // establish a uncompressed bitvector with the correct size
    res.nbits = ((nbits==0 && m_vec.size()>0) ?
		 (rhs.nbits == 0 && rhs.m_vec.size()>0) ? 
		 (m_vec.size() <= rhs.m_vec.size() ? do_cnt() : rhs.do_cnt())
		 : rhs.nbits : nbits);
    res.m_vec.resize(res.nbits/MAXBITS);

    // fill res with the right numbers
    if (m_vec.size() == 1) { // only one word in *this
	array_t<word_t>::const_iterator it = m_vec.begin();
	if (*it > HEADER1) { // complement of rhs
	    rhs.copy_comp(res.m_vec);
	    if (rhs.nset > 0)
		res.nset = nbits - rhs.nset;
	}
	else if (*it > ALLONES) { // all zero
	    decompress(res.m_vec);
	    res.nset = 0;
	}
	else {
	    res.m_vec[0] = (*it & ~*(rhs.m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (rhs.m_vec.size() == 1) { // only one word in rhs
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) { // all zero
	    (void) memset(res.m_vec.begin(), 0,
			  res.m_vec.size()*sizeof(word_t));
	    res.nset = 0;
	}
	else if (*it > ALLONES) { // copy *this
	    res.m_vec.deepCopy(m_vec);
	    res.nset = nset;
	}
	else {
	    res.m_vec[0] = (*it & ~*(m_vec.begin()));
	    res.nset = cnt_ones(res.m_vec[0]);
	}
    }
    else if (m_vec.size() > 1) { // more than one word in *this
	run x, y;
	res.nset = 0;
	x.it = m_vec.begin();
	y.it = rhs.m_vec.begin();
	array_t<word_t>::iterator ir = res.m_vec.begin();
	while (x.it < m_vec.end()) {	// go through all words in m_vec
	    if (x.nWords == 0)
		x.decode();
	    if (y.nWords == 0)
		y.decode();
	    if (x.isFill != 0) {
		if (y.isFill != 0 && y.nWords >= x.nWords) {
		    if (y.fillBit == 0) {
			res.copy_runs(ir, x, y.nWords);
			y.it += (y.nWords == 0);
		    }
		    else {
			x -= y.nWords;
			y.fillBit = 0;
			res.copy_fill(ir, y);
		    }
		}
		else if (x.fillBit == 0) {
		    y -= x.nWords;
		    res.copy_fill(ir, x);
		}
		else {
		    res.copy_runsn(ir, y, x.nWords);
		    x.it += (x.nWords == 0);
		}
	    }
	    else if (y.isFill != 0) {
		if (y.fillBit == 0) {
		    res.copy_runs(ir, x, y.nWords);
		    y.it += (y.nWords == 0);
		}
		else {
		    x -= y.nWords;
		    y.fillBit = 0;
		    res.copy_fill(ir, y);
		}
	    }
	    else { // both words are not compressed
		*ir = *x.it & ~*y.it;
		x.nWords = 0;
		y.nWords = 0;
		++ x.it;
		++ y.it;
		++ ir;
	    }
	} // while (x.it < m_vec.end())

	if (x.it != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::minus_d2 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left",
				   static_cast<long int>(m_vec.end() - x.it));
	    throw "minus_d2 internal error";
	}

	if (y.it != rhs.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::minus_d2 "
				   "expects to exhaust i1 but there are %ld "
				   "word(s) left", static_cast<long int>
				   (rhs.m_vec.end() - y.it));
	    throw "minus_d2 internal error";
	}

	if (ir != res.m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::minus_d2 "
				   "expects to exhaust ir but there are %ld "
				   "word(s) left", static_cast<long int>
				   (res.m_vec.end() - ir));
	    throw "minus_d2 internal error";
	}
    }

    // work with the two active_words
    res.active.val = active.val & ~rhs.active.val;
    res.active.nbits = active.nbits;
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "operand 1 of MINUS" << *this << std::endl;
    lg.buffer() << "operand 2 of MINUS" << rhs << std::endl;
    lg.buffer() << "result of MINUS " << res;
#endif
} // ibis::bitvector::minus_d2

// assuming *this is uncompressed but rhs is not, this function performs
// the MINUS operation and overwrites *this with the result
void ibis::bitvector::minus_d1(const ibis::bitvector& rhs) {
    m_vec.nosharing(); // make sure *this is not shared!
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "operand 1 of MINUS" << *this << std::endl;
	lg.buffer() << "operand 2 of MINUS" << rhs << std::endl;
    }
#endif
    if (rhs.m_vec.size() == 1) {
	array_t<word_t>::const_iterator it = rhs.m_vec.begin();
	if (*it > HEADER1) { // 0-fill
	    (void) memset(m_vec.begin(), 0,
			  m_vec.size()*sizeof(word_t));
	    nset = 0;
	}
	else if (*it <= ALLONES) {
	    m_vec[0] = (*it & ~*(rhs.m_vec.begin()));
	    nset = cnt_ones(m_vec[0]);
	}
    }
    else if (rhs.m_vec.size() > 1) {
	nset = 0;
	word_t s0;
	array_t<word_t>::iterator i0 = m_vec.begin();
	array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
	while (i1 != rhs.m_vec.end()) {	// go through all words in m_vec
	    if (*i1 > ALLONES) { // i1 is compressed
		s0 = ((*i1) & MAXCNT);
		if ((*i1) >= HEADER1) { // the result is 0
		    memset(i0, 0, sizeof(word_t)*s0);
		}
		i0 += s0;
	    }
	    else { // both words are not compressed
		*i0 &= ~*i1;
		++ i0;
	    }
	    ++ i1;
	} // while (i1 != rhs.m_vec.end())

	if (i0 != m_vec.end()) {
	    ibis::util::logMessage("Error", "ibis::bitvector::minus_d1 "
				   "expects to exhaust i0 but there are %ld "
				   "word(s) left", static_cast<long int>
				   (m_vec.end() - i0));
	    throw "minus_d1 internal error";
	}
    }

    // the last thing -- work with the two active_words
    active.val &= ~rhs.active.val;
#if defined(DEBUG) && DEBUG + 0 > 1
    {
	ibis::util::logger lg(ibis::gVerbose);
	lg.buffer() << "result of MINUS" << *this;
    }
#endif
} // ibis::bitvector::minus_d1

// both operands of the 'minus' operation are not compressed
void ibis::bitvector::minus_c0(const ibis::bitvector& rhs) {
    nset = 0;
    m_vec.nosharing(); // make sure *this is not shared!
    array_t<word_t>::iterator i0 = m_vec.begin();
    array_t<word_t>::const_iterator i1 = rhs.m_vec.begin();
    while (i0 != m_vec.end()) {	// go through all words in m_vec
	*i0 &= ~(*i1);
	i0++;
	i1++;
    } // while (i0 != m_vec.end())

    // the last thing -- work with the two active_words
    active.val &= ~(rhs.active.val);
#if defined(DEBUG) && DEBUG + 0 > 1
    ibis::util::logger lg(ibis::gVerbose);
    lg.buffer() << "result MINUS " << *this;
#endif
} // ibis::bitvector::minus_c0

// Adjust the size of bit sequence. if current size is less than nv,
// append enough 1 bits so that it has nv bits.  If the resulting total
// number of bits is less than nt, append 0 bits to that there are nt
// total bits.  The final result always contains nt bits.
void ibis::bitvector::adjustSize(word_t nv, word_t nt) {
    if (nbits < m_vec.size() * MAXBITS)
	nbits = do_cnt();
    if (size() == nt) return;
    m_vec.nosharing();

    if (nv > nt)
	nv = nt;
    if (size() < nv)
	appendFill(1, nv - size());
    if (size() < nt) {
	appendFill(0, nt - size());
    }
    else if (size() > nt) {
	erase(nt, size());
    }
} // ibis::bitvector::adjustSize

void ibis::bitvector::reserve(unsigned nb, unsigned nc, double cf) {
    if (nb >= nc && nc > 0 && nb > MAXBITS) {
	double sz = 0.0;
	const double den = static_cast<double>(nc) / nb;
	const word_t nw = nb / MAXBITS;
	if ((den <= 0.5 && cf > 1.00001) || (den > 0.5 && (1.0-den)*cf > den))
	    sz = ((1.0-den) * pow(1.0-den/((1.0-den)*cf),
				  static_cast<int>(2*MAXBITS-3)) +
		  den * pow(1.0-1.0/cf, static_cast<int>(2*MAXBITS-3)));
	else
	    sz = (pow(1.0-den, static_cast<int>(2*MAXBITS)) +
		  pow(den, static_cast<int>(2*MAXBITS)));
	sz = ceil(nw * (1.0 - sz));
	if (ibis::gVerbose > 9)
	    ibis::util::logMessage("bitvector::reserve",
				   "attempting to reserve %g bytes", sz);
	m_vec.reserve(3+static_cast<uint32_t>(sz));
    }
} // ibis::bitvector::reserve

// estimate the clustering factor based on the size information of a bitmap
double ibis::bitvector::clusteringFactor(word_t nb, word_t nc,
					 word_t sz) {
    double f = 1.0;
    sz /= sizeof(word_t); // internally use the number words as size measure
    if (sz <= 3) {
	f = nc;
    }
    else if (nb > MAXBITS && nc > 0 && nb >= nc && nb > MAXBITS * sz) {
	const int tw3 = MAXBITS + MAXBITS - 3;
	const double den = static_cast<double>(nc) /
	    static_cast<double>(nb);
	const word_t nw = nb / MAXBITS;
	const double f0 = (den > 0.5 ? den / (1 - den) : 1.0); // lower bound
	const double sz1 = 3.0 + nw - sz;
	double ds = 0.0;
	f = f0;
#if defined(DEBUG)
	LOGGER(0)
	    << "DEBUG -- ibis::bitvector:clusteringFactor(" << nb << ", " << nc
	    << ", " << sz << "): sz=" << sz << ", den = "
	    << den << ", nw = " << nw;
#endif
	do {
	    // This is a simple combination of the Newton's method and the
	    // secant method for finding a root.  It use the Newton's method
	    // to find the second point and then use the two point to
	    // perform an extrapolation (secant method).
	    double f2, ds2;
	    ds = sz1 - nw *
		((1.0-den) * pow(1.0-den/((1.0-den)*f), tw3) +
		 den * pow(1.0-1.0/f, tw3));
	    double deri = (tw3 * nw * den / (f*f)) *
		(pow(1.0-den/((1.0-den)*f), tw3-1) +
		 pow(1.0-1.0/f, tw3-1));
	    if (deri != 0.0) {
		ds2 = ds / deri;
		f2 = f + ds2;
		if (f2 < f0) { // undershot
		    f2 = sqrt(f0 * f);
		}
		else if (f2 > nc) { // overshot
		    f2 = sqrt(nc * f);
		}
	    }
	    else { // zero derivative, try something smaller
		f2 = sqrt(f0 * f);
	    }
	    ds2 = sz1 - nw *
		((1.0-den) * pow(1.0-den/((1.0-den)*f2), tw3) +
		 den * pow(1.0-1.0/f2, tw3));
#if defined(DEBUG)
	    LOGGER(0)
		<< "DEBUG -- ibis::bitvector:clusteringFactor(" << nb
		<< ", " << nc << ", " << sz << "): computed size="
		<< (ds + sz) << ", ds = "
		<< ds << ", deri = " << deri << ", f = "
		<< f << ", ds2 = " << ds2 << ", f2 = " << f2;
#endif
#if defined(QUARDRATIC_INTERPOLATION_FOR_CLUSTERING_FACTOR)
	    // quardratic extrapolation does not work well, likely because
	    // (1) have not found a strategy to decide which one of the two
	    // solutions to use
	    // (2) it relies on the first point more than the second point,
	    // because it uses the function value and the derivative of the
	    // first point but only use the function value of the second
	    // point.  This biases the extrapolation too much to the first
	    // point and reduces its effectiveness.
	    if (f != f2) { // quardratic extrapolation
		double tmp, a, b, c;
		c = (f - f2);
		a = - deri - (ds - ds2) / c;
		if (a != 0) {
		    a /= c;
		    b = - deri - 2.0 * a * f;
		    c = ds - (a*f + b) * f;
		    if (b*b > 4.0*a*c) { // quardratic formula has solutions
			c = 0.5 * sqrt(b*b - 4.0*a*c) / a;
			b = -0.5 * b / a;
			a = b - c;
			b += c;
			if (a > b) {
			    c = a;
			    a = b;
			    b = c;
			}
#if defined(DEBUG)
			lg.buffer() << "a = " << a << ", b = " << b
				  << ", linear extrapolation = "
				  << f - (f - f2) * ds / (ds - ds2)
				  << std::endl;
#endif
			//if (a < f0) tmp = b;
			//else tmp = a;
			tmp = b;
		    }
		    else { // use linear interpolation
			tmp = f - (f - f2) * ds / (ds - ds2);
		    }
		    if (tmp > f0) f2 = tmp;
		    else f2 = sqrt(f0 * f2);
		}
	    }
#else
	    if (ds != ds2) { // use linear interpolation
		double tmp = (f*ds2 - f2*ds) / (ds2 - ds);
		if (tmp < f0) // undershot
		    f2 = sqrt(f * f0);
		else if (tmp > nc) // overshot
		    f2 = sqrt(f * nc);
		else
		    f2 = tmp;
	    }
	    else {
		f2 = 0.5 *(f + f2);
	    }
#endif
	    ds = f2 - f;
	    f = f2;
	} while (fabs(ds) > 1e-4*f);
    }
    return f;
} // ibis::bitvector::clusteringFactor

// assignment operator for ibis::bitvector::iterator
// ********************IMPORTANT********************
// operator= modifies the content of the bitvector it points to and it can
// invalidate other iterator and const_iterator referring to the same
// bitvector.
const ibis::bitvector::iterator&
ibis::bitvector::iterator::operator=(int val) {
    if (it > vec->end()) { // end
	ibis::util::logMessage("Warning", "attempting to assign value to an "
			       "invalid bitvector::iterator");
	return *this;
    }
    if ((val!=0) == operator*()) {
	return *this; // no change
    }
    if (it == vec->end()) { // modify the active bit
	if (val != 0) {
	    active->val |= (1 << (active->nbits - ind - 1));
	}
	else {
	    active->val &= ~(1 << (active->nbits - ind - 1));
	}
	return *this;
    }

    /////////////////////////////////////
    // the bit to be modified is in m_vec
    //
    if (compressed == 0) { // toggle a single bit of a literal word
	*it ^= (1 << (ibis::bitvector::SECONDBIT - ind));
    }
    else if (ind < ibis::bitvector::MAXBITS) {
	// bit to be modified is in the first word, two pieces
	-- (*it);
	if ((*it & ibis::bitvector::MAXCNT) == 1)
	    *it = (val != 0)?0:ibis::bitvector::ALLONES;
	word_t w = (1 << (ibis::bitvector::SECONDBIT-ind));
	if (val == 0) w ^= ibis::bitvector::ALLONES;
	it = vec->insert(it, w);
    }
    else if (nbits - ind <= ibis::bitvector::MAXBITS) {
	// bit to be modified is in the last word, two pieces
	-- (*it);
	if ((*it & ibis::bitvector::MAXCNT) == 1)
	    *it = (val != 0)?0:ibis::bitvector::ALLONES;
	word_t w = 1 << (nbits-ind-1);
	if (val == 0) w ^= ibis::bitvector::ALLONES;
	++ it;
	it = vec->insert(it, w);
    }
    else { // the counter breaks into three pieces
	word_t u[2], w;
	u[0] = ind / MAXBITS;
	w = (*it & MAXCNT) - u[0] - 1;
	u[1] = 1 << (SECONDBIT - ind + u[0]*MAXBITS);
	if (val==0) {
	    u[0] = (u[0]>1)?(HEADER1|u[0]):(ALLONES);
	    u[1] ^= ibis::bitvector::ALLONES;
	    w = (w>1)?(HEADER1|w):(ALLONES);
	}
	else {
	    u[0] = (*u>1)?(HEADER0|u[0]):static_cast<word_t>(0);
	    w = (w>1)?(HEADER0|w):static_cast<word_t>(0);
	}
	*it = w;
	// NOTE: std::vector::iterator can not be subtracted like this
	w = it - vec->begin();
	vec->insert(it, u, u+2);
	++ w;
	it = vec->begin() + w;//std::vector::iterator can't be added like this
    }

    // restore the current iterator and the referred bitvector to the correct
    // states
    ind = ind % ibis::bitvector::MAXBITS;
    nbits = ibis::bitvector::MAXBITS;
    literalvalue = *it;
    compressed = 0;
    if (bitv->nset) bitv->nset += val?1:-1;
    return *this;
} // ibis::bitvector::iterator::operator=(int val)

// advance position with a longer stride
ibis::bitvector::iterator& ibis::bitvector::iterator::operator+=(int incr) {
    if (incr < 0) { // advance backword
	if (ind >= static_cast<word_t>(-incr)) {
	    ind += incr;
	}
	else { // need to move on the previous word
	    int incr0 = incr + ind;
	    while (incr0 < 0 && it > vec->begin()) {
		--it;
		decodeWord();
		if (nbits >= static_cast<word_t>(-incr0)) {
		    ind = nbits + incr0;
		    incr0 = 0;
		}
		else {
		    incr0 += nbits;
		}
	    }
	    if (incr0 < 0) {
		ibis::util::logger lg(-1);
		lg.buffer()
		    << " Error *** ibis::bitvector::iterator::operator+=("
		    << incr << ") passes the beginning of the "
		    << "bit sequence";
	    }
	}
    }
    else if (incr > 0) { // advance forward
	if (ind+incr < nbits) {
	    ind += incr;
	}
	else { // need to move on to the next word
	    int incr1 = incr + ind - nbits;
	    while (incr1 >= 0 && it < vec->end()) {
		++ it;
		decodeWord();
		if (nbits > (word_t)incr1) {
		    ind   = incr1;
		    incr1 = INT_MIN;
		}
		else {
		    incr1 -= nbits;
		}
	    }
	    if (incr1 > 0) {
		ibis::util::logger lg(-1);
		lg.buffer()
		    << " Error *** ibis::bitvector::iterator::operator+=("
		    << incr << ") passes the end of the "
		    << "bit sequence";
	    }
	}
    }

    return *this;
} // ibis::bitvector::iterator::operator+=

// decode the word pointed by it;
// when it is out of designated range, it returns end()
void ibis::bitvector::iterator::decodeWord() {
    if (it < vec->end() && it >= vec->begin()) {
	// deal with the normal case first
	if (*it > ibis::bitvector::HEADER1) {
	    fillbit = 1;
	    compressed = 1;
	    nbits = ((*it) & MAXCNT) * MAXBITS;
	}
	else if (*it > ibis::bitvector::HEADER0) {
	    fillbit = 0;
	    compressed = 1;
	    nbits = ((*it) & MAXCNT) * MAXBITS;
	}
	else {
	    compressed = 0;
	    nbits = MAXBITS;
	    literalvalue = *it;
	}
    }
    else if (it == vec->end()) { // return active word
	compressed = 0;
	nbits = active->nbits;
	literalvalue = (active->val << (MAXBITS - nbits));
	it += (nbits == 0);
    }
    else { // it is out of valid range -- treat it as the end
	it = vec->end() + 1;
	compressed = 0;
	nbits = 0;
	literalvalue = 0;
	fillbit = 0;
    }
    ind = 0;
} // ibis::bitvector::iterator::decodeWord()

// advance position with a longer stride
ibis::bitvector::const_iterator&
ibis::bitvector::const_iterator::operator+=(int incr) {
    if (incr < 0) { // advance backword
	if (ind >= static_cast<word_t>(-incr)) {
	    ind += incr;
	}
	else { // need to move on the previous word
	    int incr0 = incr + ind;
	    while (incr0 < 0 && it > begin) {
		--it;
		decodeWord();
		if (nbits >= static_cast<word_t>(-incr0)) {
		    ind = nbits + incr0;
		    incr0 = 0;
		}
		else {
		    incr0 += nbits;
		}
	    }
	    if (incr0 < 0) {
		ibis::util::logger lg(-1);
		lg.buffer() << " Error *** ibis::bitvector::const_iterator::"
			    << "operator+=(" << incr
			    << ") passes the beginning of the bit sequence";
	    }
	}
    }
    else if (incr > 0) { // advance forward
	if (ind+incr < nbits) {
	    ind += incr;
	}
	else { // need to move on to the next word
	    int incr1 = incr + ind - nbits;
	    while (incr1 >= 0 && it < end) {
		++ it;
		decodeWord();
		if (nbits > (word_t)incr1) {
		    ind   = incr1;
		    incr1 = INT_MIN;
		}
		else {
		    incr1 -= nbits;
		}
	    }
	    if (incr1 > 0) {
		ibis::util::logger lg(-1);
		lg.buffer() << " Error *** ibis::bitvector::const_iterator::"
			    << "operator+=(" << incr
			    << ") passes the end of the bit sequence";
	    }
	}
    }

    return *this;
} // ibis::bitvector::const_iterator::operator+=

// decode the word pointed by it;
// when it is out of designated range, it returns end()
void ibis::bitvector::const_iterator::decodeWord() {
    if (it < end && it >= begin) { // deal with the normal case first
	if (*it > ibis::bitvector::HEADER1) {
	    fillbit = 1;
	    compressed = 1;
	    nbits = ((*it) & MAXCNT) * MAXBITS;
	}
	else if (*it > ibis::bitvector::HEADER0) {
	    fillbit = 0;
	    compressed = 1;
	    nbits = ((*it) & MAXCNT) * MAXBITS;
	}
	else {
	    compressed = 0;
	    nbits = MAXBITS;
	    literalvalue = *it;
	}
    }
    else if (it == end) { // return active word
	compressed = 0;
	nbits = active->nbits;
	literalvalue = (active->val << (MAXBITS - nbits));
	it += (nbits == 0);
    }
    else { // it is out of valid range -- treat it as the end
	it = end + 1;
	compressed = 0; nbits = 0; literalvalue = 0; fillbit = 0;
    }
    ind = 0;
} // ibis::bitvector::const_iterator::decodeWord()

// advance to the next code word that is not zero
ibis::bitvector::indexSet& ibis::bitvector::indexSet::operator++() {
    if (it > end) { // already at the end
	nind = 0;
	return *this;
    }
    //     if (it >= end) { // reaching the end
    // 	++ it;
    // 	nind = 0;
    // 	return *this;
    //     }

    // the index of the next position
    word_t index0 = ((ind[0]+(nind>MAXBITS?nind:MAXBITS)) / MAXBITS)
	* MAXBITS;

    // skip to the next code word containing any ones
    ++ it; // examining the next word
    nind = 0;
    while (it < end) {
	if (*it >= HEADER1) { // 1-fill
	    nind = ((*it) & MAXCNT) * MAXBITS;
	    ind[1] = index0 + nind;
	    ind[0] = index0;
	    return *this;
	}
	else if (*it >= HEADER0) { // 0-fill
	    index0 += ((*it) & MAXCNT) * MAXBITS;
	    ++ it;
	}
	else if (*it > 0) { // non-zero literal word
	    if (*it < ALLONES) { // a mixture of 0 and 1
		word_t i, j = (*it<<1);
		for (i=0; j>0; ++i, j <<= 1) {
		    if (j > ALLONES) {
			ind[nind] = index0 + i;
			++ nind;
		    }
		}
	    }
	    else { // all 1s
		nind = MAXBITS;
		ind[0] = index0;
		ind[1] = index0 + nind;
		// for (int i=0; i<MAXBITS; ++i) ind[i] = index0+i;
	    }
	    return *this;
	}
	else { // zero word
	    index0 += MAXBITS;
	    ++ it;
	}
    } // while (it < end)

    // deal with the active word
    if (active->nbits > 0 && active->val > 0) {
	// a non-zero active word
	word_t i, j = (active->val << (MAXBITS + 1 - active->nbits));
	for (i=0; j>0; ++i, j <<= 1) {
	    if (j > ALLONES) {
		ind[nind] = index0 + i;
		++ nind;
	    }
	}
    }
    it = end + 1;
    return *this;
} // ibis::bitvector::indexSet::operator++
