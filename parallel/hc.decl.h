#ifndef _DECL_hc_H_
#define _DECL_hc_H_
#include "charm++.h"
#include "searchEngine.decl.h"

/* DECLS: readonly bool whitePerspective;
 */

/* DECLS: readonly uint16_t moveHistory[100];
 */

/* DECLS: readonly int maxdepth;
 */

/* DECLS: readonly vector<set<uint16_t > > failedMoves;
 */

/* DECLS: readonly long long rankFileDests[64];
 */

/* DECLS: readonly long long diagonalDests[64];
 */

/* DECLS: readonly long long knightDests[64];
 */

/* DECLS: readonly vector<uint16_t > globalState;
 */

/* DECLS: mainchare Hc: Chare{
Hc(CkArgMsg* impl_msg);
};
 */
 class Hc;
 class CkIndex_Hc;
 class CProxy_Hc;
/* --------------- index object ------------------ */
class CkIndex_Hc:public CProxy_Chare{
  public:
    typedef Hc local_t;
    typedef CkIndex_Hc index_t;
    typedef CProxy_Hc proxy_t;
    typedef CProxy_Hc element_t;

    static int __idx;
    static void __register(const char *s, size_t size);
/* DECLS: Hc(CkArgMsg* impl_msg);
 */
    static int __idx_Hc_CkArgMsg;
    static int ckNew(CkArgMsg* impl_msg) { return __idx_Hc_CkArgMsg; }
    static void _call_Hc_CkArgMsg(void* impl_msg,Hc* impl_obj);

};
/* --------------- element proxy ------------------ */
class CProxy_Hc:public CProxy_Chare{
  public:
    typedef Hc local_t;
    typedef CkIndex_Hc index_t;
    typedef CProxy_Hc proxy_t;
    typedef CProxy_Hc element_t;

    CProxy_Hc(void) {};
    CProxy_Hc(CkChareID __cid) : CProxy_Chare(__cid){  }
    CProxy_Hc(const Chare *c) : CProxy_Chare(c){  }
int ckIsDelegated(void) const {return CProxy_Chare::ckIsDelegated();}
inline CkDelegateMgr *ckDelegatedTo(void) const {return CProxy_Chare::ckDelegatedTo();}
inline CkDelegateData *ckDelegatedPtr(void) const {return CProxy_Chare::ckDelegatedPtr();}
CkGroupID ckDelegatedIdx(void) const {return CProxy_Chare::ckDelegatedIdx();}
inline void ckCheck(void) const {CProxy_Chare::ckCheck();}
const CkChareID &ckGetChareID(void) const
{ return CProxy_Chare::ckGetChareID(); }
operator const CkChareID &(void) const {return ckGetChareID();}
    void ckDelegate(CkDelegateMgr *dTo,CkDelegateData *dPtr=NULL) {
      CProxy_Chare::ckDelegate(dTo,dPtr);
    }
    void ckUndelegate(void) {
      CProxy_Chare::ckUndelegate();
    }
    void pup(PUP::er &p) {
      CProxy_Chare::pup(p);
    }
    void ckSetChareID(const CkChareID &c) {
      CProxy_Chare::ckSetChareID(c);
    }
    Hc *ckLocal(void) const
     { return (Hc *)CkLocalChare(&ckGetChareID()); }
/* DECLS: Hc(CkArgMsg* impl_msg);
 */
    static CkChareID ckNew(CkArgMsg* impl_msg, int onPE=CK_PE_ANY);
    static void ckNew(CkArgMsg* impl_msg, CkChareID* pcid, int onPE=CK_PE_ANY);
    CProxy_Hc(CkArgMsg* impl_msg, int onPE=CK_PE_ANY);

};
PUPmarshall(CProxy_Hc)
typedef CBaseT1<Chare, CProxy_Hc> CBase_Hc;

extern void _registerhc(void);
extern "C" void CkRegisterMainModule(void);
#endif
