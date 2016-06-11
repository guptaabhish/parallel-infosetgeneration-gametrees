
/* DEFS: readonly bool whitePerspective;
 */
extern bool whitePerspective;
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_whitePerspective(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p|whitePerspective;
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly uint16_t moveHistory[100];
 */
extern uint16_t moveHistory[100];
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_moveHistory(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p(moveHistory,100);
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly int maxdepth;
 */
extern int maxdepth;
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_maxdepth(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p|maxdepth;
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly vector<set<uint16_t > > failedMoves;
 */
extern vector<set<uint16_t > > failedMoves;
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_failedMoves(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p|failedMoves;
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly long long rankFileDests[64];
 */
extern long long rankFileDests[64];
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_rankFileDests(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p(rankFileDests,64);
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly long long diagonalDests[64];
 */
extern long long diagonalDests[64];
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_diagonalDests(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p(diagonalDests,64);
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly long long knightDests[64];
 */
extern long long knightDests[64];
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_knightDests(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p(knightDests,64);
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly vector<uint16_t > globalState;
 */
extern vector<uint16_t > globalState;
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_globalState(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p|globalState;
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: mainchare Hc: Chare{
Hc(CkArgMsg* impl_msg);
};
 */
#ifndef CK_TEMPLATES_ONLY
 int CkIndex_Hc::__idx=0;
/* DEFS: Hc(CkArgMsg* impl_msg);
 */
CkChareID CProxy_Hc::ckNew(CkArgMsg* impl_msg, int impl_onPE)
{
  CkChareID impl_ret;
  CkCreateChare(CkIndex_Hc::__idx, CkIndex_Hc::__idx_Hc_CkArgMsg, impl_msg, &impl_ret, impl_onPE);
  return impl_ret;
}
void CProxy_Hc::ckNew(CkArgMsg* impl_msg, CkChareID* pcid, int impl_onPE)
{
  CkCreateChare(CkIndex_Hc::__idx, CkIndex_Hc::__idx_Hc_CkArgMsg, impl_msg, pcid, impl_onPE);
}
  CProxy_Hc::CProxy_Hc(CkArgMsg* impl_msg, int impl_onPE)
{
  CkChareID impl_ret;
  CkCreateChare(CkIndex_Hc::__idx, CkIndex_Hc::__idx_Hc_CkArgMsg, impl_msg, &impl_ret, impl_onPE);
  ckSetChareID(impl_ret);
}
 int CkIndex_Hc::__idx_Hc_CkArgMsg=0;
void CkIndex_Hc::_call_Hc_CkArgMsg(void* impl_msg,Hc * impl_obj)
{
  new (impl_obj) Hc((CkArgMsg*)impl_msg);
}

#endif /* CK_TEMPLATES_ONLY */
#ifndef CK_TEMPLATES_ONLY
void CkIndex_Hc::__register(const char *s, size_t size) {
  __idx = CkRegisterChare(s, size, TypeMainChare);
  CkRegisterBase(__idx, CkIndex_Chare::__idx);
// REG: Hc(CkArgMsg* impl_msg);
  __idx_Hc_CkArgMsg = CkRegisterEp("Hc(CkArgMsg* impl_msg)",
     (CkCallFnPtr)_call_Hc_CkArgMsg, CMessage_CkArgMsg::__idx, __idx, 0);
  CkRegisterMainChare(__idx, __idx_Hc_CkArgMsg);

}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
void _registerhc(void)
{
  static int _done = 0; if(_done) return; _done = 1;
      _registersearchEngine();

  CkRegisterReadonly("whitePerspective","bool",sizeof(whitePerspective),(void *) &whitePerspective,__xlater_roPup_whitePerspective);

  CkRegisterReadonly("moveHistory","uint16_t",sizeof(moveHistory),(void *) &moveHistory,__xlater_roPup_moveHistory);

  CkRegisterReadonly("maxdepth","int",sizeof(maxdepth),(void *) &maxdepth,__xlater_roPup_maxdepth);

  CkRegisterReadonly("failedMoves","vector<set<uint16_t > >",sizeof(failedMoves),(void *) &failedMoves,__xlater_roPup_failedMoves);

  CkRegisterReadonly("rankFileDests","long long",sizeof(rankFileDests),(void *) &rankFileDests,__xlater_roPup_rankFileDests);

  CkRegisterReadonly("diagonalDests","long long",sizeof(diagonalDests),(void *) &diagonalDests,__xlater_roPup_diagonalDests);

  CkRegisterReadonly("knightDests","long long",sizeof(knightDests),(void *) &knightDests,__xlater_roPup_knightDests);

  CkRegisterReadonly("globalState","vector<uint16_t >",sizeof(globalState),(void *) &globalState,__xlater_roPup_globalState);

/* REG: mainchare Hc: Chare{
Hc(CkArgMsg* impl_msg);
};
*/
  CkIndex_Hc::__register("Hc", sizeof(Hc));

}
extern "C" void CkRegisterMainModule(void) {
  _registerhc();
}
#endif /* CK_TEMPLATES_ONLY */
