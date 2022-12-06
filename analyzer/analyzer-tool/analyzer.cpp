#include <llvm/IR/LegacyPassManager.h>	
#include <llvm/IR/LLVMContext.h>		
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>	
#include <llvm/Support/Signals.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/SystemUtils.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/TypeFinder.h>
#include <llvm/IR/IntrinsicInst.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <string.h>

using namespace std;
using namespace llvm;

static LLVMContext global_context;

typedef struct {
	map<int, set<Function *>> callersForDepth;
	int depth;   // depth
	int callers; // number of call sites
} RetbindAttr;

map<llvm::Function *, RetbindAttr> retbindFirstFunctions;
void findRetbindTargetFunctions(llvm::Module *module);
void resolveRetbindFirstDepth(Module *module, int threshold);
void collectObjbindStructs(Module *module, char *structName, bool pointerOnly);
void diversityScoreForGlobalVariables(Module *module, StructType *type, unsigned int fieldIdx, uint64_t fieldOffset, bool globalAddrBind);

typedef struct {
	unsigned int score;
	unsigned int nonMeasurableScore;
	unsigned int undefScore;
	set<Value *> value;
	set<StructType *> types;
} DiversityScore;

typedef struct {
	map<int, uint64_t> candidateFields; // key: field idx / value: byte index
	map<int, DiversityScore> score; // divsersity score.  key: field idx / value: diversity score
} ObjbindAttr;

typedef struct {
	StoreInst *insn;
	Function *func;
	Argument *arg;
	StructType *type;
	int argIdx;
	int fieldIdx;
	uint64_t fieldOffset;
}ObjbindFirstRoundAttr;

typedef struct {
	StructType *type;
	Argument *arg;
	int argIdx;
	int fieldIdx;
	uint64_t fieldOffset;
}ObjbindWorklistAttr;

map<llvm::StructType *, ObjbindAttr> objbindStructs;
map<StructType *, unsigned int> objbindGlobalAddrBindScores;

// key: structure type which includes struct.kobject as a field
// value: field index
map<StructType *, int> kobjectTypes;

// ============== common codes ======================
static LLVMContext &getGlobalContext() {
	return global_context;
}

Value *getGEPPointerOperand(Value *V) {
	if (V == NULL)
		return NULL;
	if (auto GEP = dyn_cast_or_null<GetElementPtrInst>(V)) {
		return GEP->getPointerOperand();
	} else if (auto BCI = dyn_cast_or_null<BitCastInst>(V)) {
		Value *v2 = BCI->getOperand(0);
		if (v2 == NULL)
			return NULL;
		if (auto GEP = dyn_cast_or_null<GetElementPtrInst>(v2)) {
			return GEP->getPointerOperand();
		}
	}
	return NULL;
}

StructType *getsStructPtrType(Value *V)
{
	Type *T = NULL;			// Pointer
	Type *CT = NULL;		// Type that Pointer points
	StructType *ST = NULL;
	
	if (V == NULL)
		return NULL;

	T = V->getType();
	if (T != NULL && T->isPointerTy() == false)
		return NULL;

	CT = T->getPointerElementType();
	if (CT != NULL && CT->isPointerTy() == false) {
		return NULL;
	}

	CT = CT->getPointerElementType();
	if (CT != NULL && CT->isStructTy() == false) {
		return NULL;
	}

	ST = dyn_cast<StructType>(CT);
	return ST;
}

StructType *getStructType(Value *V) {
	Type *T = NULL;			// Pointer
	Type *CT = NULL;		// Type that Pointer points
	StructType *ST = NULL;

	T = V->getType();
	if (T->isStructTy()) {
		ST = dyn_cast<StructType>(CT);
		return ST;
	}

	if (T != NULL && T->isPointerTy() == false)
		return NULL;

	CT = T->getPointerElementType();
	if (CT != NULL && CT->isStructTy() == false) {
		return NULL;
	}

	ST = dyn_cast<StructType>(CT);
	return ST;
}

StructType *getStructTypeFromType(Type *T) {
	Type *CT = NULL;		// Type that Pointer points
	Type *CCT = NULL;
	StructType *ST = NULL;

	if (T != NULL && T->isPointerTy() == false)
		return NULL;

	CT = T->getPointerElementType();
	if (CT != NULL && CT->isStructTy() == false) {
		return NULL;
	}

	ST = dyn_cast<StructType>(CT);
	return ST;
}

StructType *getBaseStructPtrType(Value *V)
{
	if (V == NULL)
		return nullptr;

	// bci alias
	BitCastInst *bci = dyn_cast<BitCastInst>(V);
	if (bci)
		V = bci->getOperand(0);

	if (auto GEP = dyn_cast_or_null<GetElementPtrInst>(V)) {
		Value *BasePtr = GEP->getPointerOperand();
		if (auto GEP2 = dyn_cast_or_null<GetElementPtrInst>(BasePtr)) {
			BasePtr = GEP2->getPointerOperand();
		}

		if (auto AI = dyn_cast_or_null<AllocaInst>(BasePtr)) {
			StructType *ST = getsStructPtrType(AI->getOperand(0));
			return ST;
			//return nullptr;
		}
		if (auto LI = dyn_cast_or_null<LoadInst>(BasePtr)) {
			StructType *ST = getsStructPtrType(LI->getPointerOperand());
			return ST;
		}
		if (dyn_cast_or_null<Instruction>(BasePtr) == NULL) {
			StructType *ST = getStructType(BasePtr);
			return ST;
		}
		if (PHINode *phi = dyn_cast_or_null<PHINode>(BasePtr)) {
			unsigned len = phi->getNumIncomingValues();
			if (len > 1) {
				Value *v0 = phi->getIncomingValue(0);
				Value *v1 = phi->getIncomingValue(1);

				if (v0->getType()->isPointerTy()) {
					StructType *ST = getStructType(v0);
					if (ST) return ST;
				}
				if (v1->getType()->isPointerTy()) {
					StructType *ST = getStructType(v1);
					if (ST) return ST;
				}
			}
			return nullptr;
		}
	}

	return nullptr;
}

void printAllFunctionName(llvm::Module *module) {
	const auto &functions = module->getFunctionList();
	uint32_t numFuncs = functions.size();

	for (const auto &iter : functions) {
		if (iter.hasName())
			std::cout << iter.getName().data() << std::endl;
	}
}

bool findFunctionName(Module *module, const char *name) {
	const auto &functions = module->getFunctionList();

	for (const auto &iter : functions) {
		if (iter.hasName())
			if (strcmp(iter.getName().data(), name) == 0)
				return true;
	}

	return false;
}

// it recursively tracks to determine a given type is a function pointer
bool isFunctionPointerType(Type *type) {
	if (PointerType *pointerType=dyn_cast<PointerType>(type)){
		return isFunctionPointerType(pointerType->getElementType());
	}
	else if (type->isFunctionTy()){
		return  true;
	}
	return false;
}

Type* getFunctionPointerType(Type *type) {
	if (type == NULL)
		return NULL;
	
	if (PointerType *pointerType=dyn_cast<PointerType>(type)){
		return getFunctionPointerType(pointerType->getElementType());
	}
	else if (type->isFunctionTy()){
		return type;
	}

	return NULL;
}

bool hasFunctionPointerAsArgument(Function *func) {
	for (Function::arg_iterator ait = func->arg_begin(), aite = func->arg_end(); ait != aite; ++ait) {
		Argument *arg = &*ait;
		Type *type = arg->getType();

		if (isFunctionPointerType(type)) {
			return true;
		}
	}
	return false;
}

bool hasCommonFunctionPointerTypeAsArgument(Function *func1, Function *func2) {
	for (Function::arg_iterator ait = func1->arg_begin(), aite = func1->arg_end(); ait != aite; ++ait) {
		Argument *arg = &*ait;
		Type *type = arg->getType();
		Type *func1Type = getFunctionPointerType(type);

		if (func1Type) {
			for (Function::arg_iterator ait2 = func2->arg_begin(), aite2 = func2->arg_end(); ait2 != aite2; ++ait2) {
				Argument *arg = &*ait;
				Type *type = arg->getType();
				Type *func2Type = getFunctionPointerType(type);

				if (func1Type == func2Type)
					return true;
			}
		}
	}
	return false;
}
// ======================================================

// ============== statistics ======================

class StaticSignature {
public:
	Type *typesig;
	StructType *objtype;

public:
	StaticSignature() {
		typesig = NULL;
		objtype = NULL;
	}

	StaticSignature & operator=(const StaticSignature &rhs) {
		if (this != &rhs) {
			this->typesig = rhs.typesig;
			this->objtype = rhs.objtype;
		}
		return *this;
	}

	bool operator==(const StaticSignature &other) {
		return (this->typesig == other.typesig) && (this->objtype == other.objtype);
	}

	bool operator!=(const StaticSignature &other) {
		return !(*this == other);
	}

	bool operator<(const StaticSignature &other) const {
		if (this->objtype != other.objtype)
			return (unsigned long)this->objtype < (unsigned long)other.objtype;
		else
			return (unsigned long)this->typesig < (unsigned long)other.typesig;
	}
};

typedef struct {
	Function *caller;
	Function *func;
	unsigned int gen;	// the number of GEN (PAC) sites
	unsigned int use;	// the number of USE (AUT) sites
	set<Value *> genValueSet;
} StatInfo;

map<StaticSignature, StatInfo> fptrTypes;
set<StructType *> sortedTypesForObjbind;

void addStatInfo(Function *curFunc, Value *val, bool isGen, StructType *structType, bool objtype, unsigned int global_gen) {
	if (val) {
		// check asm
		{
			InlineAsm *iasm = dyn_cast<InlineAsm>(val);
			if (iasm) {
				return;
			}
		}

		Type *type = getFunctionPointerType(val->getType());
		if (type) {
			StaticSignature sig;
			sig.typesig = type;
			if (objtype) {
				sig.objtype = structType;
			}
			else
				sig.objtype = NULL;

			auto it = fptrTypes.find(sig);
			if (it != fptrTypes.end()) {
				if (isGen) {
					auto vit = it->second.genValueSet.find(val);
					if (vit == it->second.genValueSet.end()) {
						it->second.genValueSet.insert(val);
						if (global_gen > 0)
							it->second.gen += global_gen;
						else
							it->second.gen += 1;
					}
				}
				else it->second.use += 1;

				if (it->second.func == NULL) {
					Function *func = dyn_cast<Function>(val);
					if (func) it->second.func = func;
				}
			}
			else {			
				StatInfo info;
				Function *func = dyn_cast<Function>(val);
				if (func) info.func = func;
				else info.func = NULL;
				if (curFunc) info.caller = curFunc;
				else info.caller = NULL;
				info.gen = 0;
				info.use = 0;
				if (isGen) {
					info.genValueSet.insert(val);
					if (global_gen >0)
						info.gen += global_gen;
					else
						info.gen += 1;
				}
				else info.use += 1;

				StaticSignature sig;
				sig.typesig = type;
				if (objtype) {
					sig.objtype = structType;
				}
				else
					sig.objtype = NULL;
				
				fptrTypes.insert(make_pair(sig, info));
			}
		}
	}
}

void getBciAlias(Function *curFunc, map<Value *, StructType *> &bciAlias) {
	for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
		for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
			Instruction *I = &*BI++;
			BitCastInst *BCI = dyn_cast<BitCastInst>(I);

			if (BCI) {
				Value *src = BCI->getOperand(0);
				Type *dest = BCI->getDestTy();
				StructType *ST = NULL;

				if (src == NULL || dest == NULL)
					continue;

				if (dest->isPointerTy()) {
					dest = dest->getPointerElementType();
					ST = dyn_cast<StructType>(dest);
				}

				if (ST) {
					if (bciAlias.find(src) == bciAlias.end())
						bciAlias.insert(make_pair(src, ST));
				}
			}
		}
	}
}

void incrementAllowedTargetMap(unsigned int allowedTarget, map<unsigned int, unsigned int> &atMap) {
	if (allowedTarget == 0)
		allowedTarget = 1;
	auto it = atMap.find(allowedTarget);
	if (it == atMap.end()) {
		atMap.insert(make_pair(allowedTarget, 1));
	} else {
		it->second += 1;
	}
}

void doStatistics(Module *module, unsigned int thresholdAllowedTarget, bool objtype) {
	// ========================== Statistics =============================================
	// 1. analyze GEN and USE
	unsigned total_func = module->getFunctionList().size();
	unsigned current = 0;
	FILE *fp = NULL, *fp_ret = NULL;
	FILE *before_runtime_ctx = NULL, *after_runtime_ctx = NULL;
	FILE *before_ctx_graph = NULL, *after_ctx_graph = NULL;
	FILE *before_runtime_ctx_objbind = NULL;

	fp = fopen("objbind_structs.csv", "w");
	fp_ret = fopen("retbind_functions.csv", "w");
	before_runtime_ctx = fopen("before_runtime_ctx.csv", "w");
	before_runtime_ctx_objbind = fopen("before_runtime_ctx_objbind.csv", "w");
	after_runtime_ctx = fopen("after_runtime_ctx.csv", "w");
	before_ctx_graph = fopen("before_ctx_graph.csv", "w");
	after_ctx_graph = fopen("after_ctx_graph.csv", "w");

	// objbind (just collection)
	unsigned i = 0;
	collectObjbindStructs(module, NULL, true);
	for (auto it: objbindStructs) {
		diversityScoreForGlobalVariables(module, it.first, 0, 0, true);
		i++;
		if (i % 100 == 0)
			cout << "[OBJBIND][GLOBAL] Progress: " << i << " / " << objbindStructs.size() << endl;
	}

	// retbind
	retbindFirstFunctions.clear();
	findRetbindTargetFunctions(module);
	resolveRetbindFirstDepth(module, 0);

	for (auto curFunc = module->getFunctionList().begin(),
              endFunc = module->getFunctionList().end(); 
              curFunc != endFunc; ++curFunc) {
		if (current % 100 == 0) {
			cout << "[PROGRESS] " << current << " / " << total_func << endl;
		}

		for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
			for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
				Instruction *I = &*BI++;
				StoreInst *store = dyn_cast<StoreInst>(I);
				CallInst *call = dyn_cast<CallInst>(I);
				LoadInst *load = dyn_cast<LoadInst>(I);

				// logging
				/*
				string str;
				raw_string_ostream output(str);
				I->print(output);
				cout << str << endl; */

				// Check GEN
				if (store) {
					unsigned int global_count = 0;

					StructType *type = getBaseStructPtrType(store->getPointerOperand());
					if (type == NULL) {
						BitCastInst *bci = dyn_cast<BitCastInst>(store->getPointerOperand());
						if (bci) {
							type = getStructTypeFromType(bci->getDestTy());
						}
					}

					addStatInfo(&*curFunc, store->getValueOperand(), true, type, objtype, 0);
				}
				// Check USE
				else if (call) {
					Function *func = call->getCalledFunction();

					// Check GEN
					if (func) {
						int idx = 0;
						for (Function::arg_iterator ait = func->arg_begin(), aite = func->arg_end(); ait != aite; ++ait) {
							Argument *arg = &*ait;
							Value *callArg = call->getArgOperand(idx);
							StructType *type = NULL;
							
							if (callArg) {
								type = getBaseStructPtrType(callArg);
							}

							addStatInfo(&*curFunc, (Value *)arg, true, type, objtype, 0);
							idx++;
						}
					}
					// function pointer use. Check USE
					else {
						StructType *type = NULL;
						Value *v = call->getCalledOperand();

						type = getBaseStructPtrType(v);
						if (type == NULL) {
							BitCastInst *bci = dyn_cast<BitCastInst>(v);
							LoadInst *LI = dyn_cast_or_null<LoadInst>(v);

							if (bci || LI) {
								if (bci) {
									LI = dyn_cast<LoadInst>(bci->getOperand(0));
								}
								if (LI) {
									Value *V = LI->getPointerOperand();
									type = getBaseStructPtrType(V);  // needs bciAlias
								}
							}
						}

						// is it direct call?
						CallInst *dcall_check = dyn_cast<CallInst>(v);
						if (dcall_check && dcall_check->getCalledOperand()) {
							;
						} else {
							BitCastInst *bci = dyn_cast<BitCastInst>(v);
							LoadInst *li = dyn_cast_or_null<LoadInst>(v);
							if (bci || li) {
								addStatInfo(&*curFunc, v, false, type, objtype, 0);
							}
						}
					}
				} else if (load) {
					// Repac case; +use only
					Value *v = load->getPointerOperand();
					if (isFunctionPointerType(v->getType())) {
						StructType *type = getBaseStructPtrType(v);
						addStatInfo(&*curFunc, v, false, type, objtype, 0);
					}
				}
			}
		}

		current++;
	}

	// adds global GEN count
	for (auto ty = fptrTypes.begin(); ty != fptrTypes.end(); ++ty) {
		StructType *st = ty->first.objtype;
		auto global_it = objbindGlobalAddrBindScores.find(st);
		if (global_it != objbindGlobalAddrBindScores.end()) {
			ty->second.gen += global_it->second;
		}
	}

	// 2. sorting
	vector<pair<StaticSignature, StatInfo>> sorted(fptrTypes.begin(), fptrTypes.end());
	std::sort(sorted.begin(), sorted.end(), [](pair<StaticSignature, StatInfo> a, pair<StaticSignature, StatInfo> b) {
		// [TODO] which metric here?
		//return (a.second.gen + a.second.use) > (b.second.gen + b.second.use);
		return (a.second.gen * a.second.use) > (b.second.gen * b.second.use);
	});
	// ==================================================================================================================

	// ============================ Reporting ==============================
	// 4. reporting
	unsigned int idx = 0;

	cout << "Stat: " << endl;
	cout << "\t" << "Total: " << sorted.size() << endl;
	cout << "\t" << "Threshold allowed target: " << thresholdAllowedTarget << endl;
	cout << endl;

	// 4-1. not considering runtime context
	map<unsigned int, unsigned int> beforeAllowedTargetMap; // key: gen * use, val: number of types
	map<unsigned int, unsigned int> afterAllowedTargetMap;

	for (auto it: sorted) {
		string str, objtype, caller;
		char out_str[1024] = {0,};
		unsigned int new_gen = 0, new_use = 0;
		raw_string_ostream output(str);

		if ((it.second.gen * it.second.use) < thresholdAllowedTarget) {
			incrementAllowedTargetMap(it.second.gen * it.second.use, beforeAllowedTargetMap);
			incrementAllowedTargetMap(it.second.gen * it.second.use, afterAllowedTargetMap);
			continue;
		}

		new_gen = it.second.gen;
		new_use = it.second.use;

		if (it.second.func) {
			str = it.second.func->getName();
		}
		else
			str = "none";

		if (it.second.caller) {
			caller = it.second.caller->getName();
		} else
			caller = "none";

		if (it.first.objtype) {
			objtype = "[objtype] Type: " + string(it.first.objtype->getName().data());
			auto sorted_it = sortedTypesForObjbind.find(it.first.objtype);
			if (sorted_it == sortedTypesForObjbind.end()) {
				sortedTypesForObjbind.insert(it.first.objtype);
				if (fp) {
					/*
					stringstream ss(it.first.objtype->getName());
					string token;
					string output = "";

					getline(ss, token, '.');
					output += token;

					getline(ss, token, '.');
					output += ".";
					output += token;
					output += "\n"; */
					string output = string(it.first.objtype->getName()) + "\n";

					fwrite(output.data(), strlen(output.data()), 1, fp);
				}
			}

			// exclude it
			new_gen = 1;
			new_use = 1;
		}
		else {
			objtype = "[typesig] Func: " + str + ", Caller: " + caller;

			for (auto rit = retbindFirstFunctions.begin(); rit != retbindFirstFunctions.end(); rit++) {
				if (it.second.func) {
					Function *F = rit->first;
					Type *ftype = it.second.func->getType();

					unsigned idx = 0;
					for (Function::arg_iterator ait = F->arg_begin(), aite = F->arg_end(); ait != aite; ++ait) {
						Argument *arg = &*ait;
						Type *type = arg->getType();

						if (isFunctionPointerType(type)) {
							if (ftype == type) {
								char retbind_str[512] = {0,};
								sprintf(retbind_str, "%s,%d,%d\n", F->getName().data(), rit->second.depth, rit->second.callers);
								fwrite(retbind_str, strlen(retbind_str), 1, fp_ret);

								// exclude it
								if (it.second.gen > rit->second.callers)
									new_gen = it.second.gen - rit->second.callers;
								else
									new_gen = 1;
							}
						}
						idx++;
					}
				}
			}
		}

		cout << objtype << " / GEN: " << it.second.gen << " / USE: " << it.second.use << " / AllowedTarget: " << (it.second.gen * it.second.use) << endl;
		sprintf(out_str, "%s / GEN: %d / USE: %d / AllowedTarget: %d\n", objtype.data(), it.second.gen, it.second.use, (it.second.gen * it.second.use));
		incrementAllowedTargetMap(it.second.gen * it.second.use, beforeAllowedTargetMap);
		if (before_runtime_ctx)
			fwrite(out_str, strlen(out_str), 1, before_runtime_ctx);
		if (before_runtime_ctx_objbind && it.first.objtype)
			fwrite(out_str, strlen(out_str), 1, before_runtime_ctx_objbind);
		idx++;

		if ((new_gen * new_use) >= thresholdAllowedTarget) {
			incrementAllowedTargetMap(new_gen * new_use, afterAllowedTargetMap);
			sprintf(out_str, "%s / GEN: %d / USE: %d / AllowedTarget: %d\n", objtype.data(), new_gen, new_use, (new_gen * new_use));
			if (after_runtime_ctx)
				fwrite(out_str, strlen(out_str), 1, after_runtime_ctx);
		}
	}
	cout << "\t" << "Reported: " << idx << endl;

	// Print csv for visualizing graph (for paper)
	vector<pair<unsigned int, unsigned int>> beforeMapSorted(beforeAllowedTargetMap.begin(), beforeAllowedTargetMap.end());
	vector<pair<unsigned int, unsigned int>> afterMapSorted(afterAllowedTargetMap.begin(), afterAllowedTargetMap.end());
	std::sort(beforeMapSorted.begin(), beforeMapSorted.end(), [](pair<unsigned int, unsigned int> a, pair<unsigned int, unsigned int> b) {
		return a.first < b.first;
	});
	std::sort(afterMapSorted.begin(), afterMapSorted.end(), [](pair<unsigned int, unsigned int> a, pair<unsigned int, unsigned int> b) {
		return a.first < b.first;
	});

	for (auto it: beforeMapSorted) {
		char out[512] = {0,};
		sprintf(out, "%d,%d\n", it.first, it.second);
		if (before_ctx_graph)
			fwrite(out, strlen(out), 1, before_ctx_graph);
	}
	for (auto it: afterMapSorted) {
		char out[512] = {0,};
		sprintf(out, "%d,%d\n", it.first, it.second);
		if (after_ctx_graph)
			fwrite(out, strlen(out), 1, after_ctx_graph);
	}

	if (fp)
		fclose(fp);
	if (fp_ret)
		fclose(fp_ret);
	if (before_runtime_ctx)
		fclose(before_runtime_ctx);
	if (before_runtime_ctx_objbind)
		fclose(before_runtime_ctx_objbind);
	if (after_runtime_ctx)
		fclose(after_runtime_ctx);
	if (before_ctx_graph)
		fclose(before_ctx_graph);
	if (after_ctx_graph)
		fclose(after_ctx_graph);	
}

// ======================================================

// ============== retbind analysis ======================

void findRetbindTargetFunctions(llvm::Module *module) {
	for (auto curFunc = module->getFunctionList().begin(), 
              endFunc = module->getFunctionList().end(); 
              curFunc != endFunc; ++curFunc) {
		//raw_ostream &output = outs();
		//curFunc->print(output);

		bool isTargetFunc = false;
		vector<Argument *> fptrVec;

		fptrVec.clear();
		for (Function::arg_iterator ait = curFunc->arg_begin(), aite = curFunc->arg_end(); ait != aite; ++ait) {
			Argument *arg = &*ait;
			Type *type = arg->getType();

			if (isFunctionPointerType(type)) {
				fptrVec.push_back(arg);
			}
		}

		if (fptrVec.size() > 0) {
			// Scan all instructions within a function and check if arg(function pointer) is actually invoked
			for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
				for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
					Instruction *I = &*BI++;

					if (CallInst *CI = dyn_cast<CallInst>(I)) {
						Function *CF = CI->getCalledFunction();
						if (!CF) {
							Value *V = CI->getCalledOperand();

							for (auto arg_it = fptrVec.begin(); arg_it != fptrVec.end(); arg_it++) {
								Argument *arg = *arg_it;
								if (arg == V) {
									isTargetFunc = true;
								}
							}
						}
					}

					if (isTargetFunc)
						break;
				}
				if (isTargetFunc)
					break;
			}
		}

		if (isTargetFunc) {
			Function *F = &*curFunc;
			RetbindAttr attr;
			attr.depth = 0;
			attr.callers = 0;
			retbindFirstFunctions.insert(make_pair(F, attr));
		}
	}

	//cout << "the number of first retbind target functions: " << retbindFirstFunctions.size() << endl;
}

void resolveRetbindFirstDepth(Module *module, int threshold) {
	cout << "start: resolveRetbindFirstDepth" << endl;

	for (auto curFunc = module->getFunctionList().begin(), 
              endFunc = module->getFunctionList().end(); 
              curFunc != endFunc; ++curFunc) {
		for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
			for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
				// Scan all CallInst instructions
				Instruction *I = &*BI++;

				if (CallInst *CI = dyn_cast<CallInst>(I)) {
					Function *CF = CI->getCalledFunction();
					auto iter = retbindFirstFunctions.find(CF);
					if (iter != retbindFirstFunctions.end()) {
						iter->second.depth = 1;
						iter->second.callers += 1;
					}
				}
			}
		}
	}

	// exclude if it has less callers than threshold
	vector<Function *> excludes;
	for (auto it = retbindFirstFunctions.begin(); it != retbindFirstFunctions.end(); it++) {
		if (it->second.callers <= threshold) {
			Function *F = it->first;
			excludes.push_back(F);
		}
	}
	for (auto it = excludes.begin(); it != excludes.end(); it++) {
		auto func = retbindFirstFunctions.find(*it);
		if (func != retbindFirstFunctions.end())
			retbindFirstFunctions.erase(func);
	}

	cout << "the number of first retbind target functions (threshold: " << threshold << ") : " << retbindFirstFunctions.size() << endl;
	cout << "end: resolveRetbindFirstDepth" << endl;
}

// func: target function
// depth: start depth == 2
void retbindResolveDepth(Module *module, Function *func, int depth) {
	// Worklist algorithm to resolve an appropriate depth for retbind
	set<Function *> worklist;
	set<Function *> updated;
	int threshold_depth = 6;

	worklist.clear();
	updated.clear();

	// Set initial worklist
	worklist.insert(func);

	// Main loop (worklist algorithm)
	while (depth < threshold_depth && worklist.size() > 0) {
		for (auto curFunc = module->getFunctionList().begin(), 
				endFunc = module->getFunctionList().end(); 
				curFunc != endFunc; ++curFunc) {
			for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
				for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
					Instruction *I = &*BI++;

					if (CallInst *CI = dyn_cast<CallInst>(I)) {
						Function *CF = CI->getCalledFunction();
						if (worklist.find(CF) != worklist.end()) {
							updated.insert(&*curFunc);
						}
					}
				}
			}
		}

		int current_depth_callers = updated.size();
		auto it = retbindFirstFunctions.find(func);
		if (it->second.callers < current_depth_callers) {
			it->second.callers = current_depth_callers;
			it->second.depth = depth;
		}
		
		// prepare next round loop
		depth += 1;
		worklist.clear();
		worklist = updated;
		updated.clear();

		// exit condition: check if functions in worklist are function_pointer-carrying ones.
		//                 exclude functions that are not carrying a function pointer.
		//                 if none of functions are in worklist afterwards, exit the loop.
		vector<Function *> excludes;
		for (auto it = worklist.begin(); it != worklist.end(); it++) {
			if (hasFunctionPointerAsArgument(*it) == false) {
				excludes.push_back(*it);
			} else {
				if (hasCommonFunctionPointerTypeAsArgument(func, *it) == false) {
					excludes.push_back(*it);
				}
			}
		}
		for (auto it = excludes.begin(); it != excludes.end(); it++) {
			auto func = worklist.find(*it);
			if (func != worklist.end())
				worklist.erase(func);
		}

		// update callersForDepth for future reporting
		set<Function *> tmp = worklist;
		it->second.callersForDepth.insert(make_pair(depth - 1, tmp));
	}
}

void printRetbindResult(int threshold) {
	FILE *fp = NULL;
	fp = fopen("retbind_score.csv", "w");

	cout << "===== Retbind Analysis Result (threshold: " << threshold << ") =====" << endl;
	for (auto it = retbindFirstFunctions.begin(); it != retbindFirstFunctions.end(); it++) {
		if (it->second.callers > threshold) {
			Function *F = it->first;
			if (it->second.depth == 1) {
				cout << "[" << F->getName().data() << "] callers: " << it->second.callers << ", depth: " << it->second.depth << endl;

				// For now, prints only those of having depth-1.
				if (fp) {
					char *name = (char *)F->getName().data();
					if (name) {
						char str[256] = {0,};
						sprintf(str, "%s,%d,%d\n", name, it->second.depth, it->second.callers);
						fwrite(str, strlen(str), 1, fp);
					}
				}
			} else {
				cout << "[" << F->getName().data() << "] callers: " << it->second.callers << ", depth: " << it->second.depth << endl;

				for (auto dmap = it->second.callersForDepth.begin(); dmap != it->second.callersForDepth.end(); dmap++) {
					cout << "depth-" << dmap->first << " ::: " << dmap->second.size() << " functions" << endl;
					for (auto func = dmap->second.begin(); func != dmap->second.end(); func++)
						cout << "\t" << (*func)->getName().data() << endl;
				}
			}
		}
	}

	if (fp)
		fclose(fp);
}

void retbindAnalysis(llvm::Module *module, int threshold) {
	// initialize
	int threshold_callers = threshold;
	retbindFirstFunctions.clear();

	// analysis
	findRetbindTargetFunctions(module);
	resolveRetbindFirstDepth(module, 0);  // caclulate with zero threshold but report with 20 threshold

	cout << "start: retbindResolveDepth" << endl;
	int idx = 0;
	for (auto it = retbindFirstFunctions.begin(); it != retbindFirstFunctions.end(); it++) {
		Function *func = it->first;
		retbindResolveDepth(module, func, 2);

		cout << "progress: func-" << idx << ": " << func->getName().data() << endl;
		idx++;
	}
	cout << "end: retbindResolveDepth" << endl;

	// report
	printRetbindResult(threshold_callers);
}
// ======================================================

// ============== objbind analysis ======================

uint64_t getFieldOffsetInByte(Module *module, StructType *type, int fieldIdx) {
	return module->getDataLayout().getStructLayout(type)->getElementOffset(fieldIdx);
}

bool isKobjectType(StructType *type) {
	if (strncmp(type->getName().data(), "struct.kobject", 14) == 0) {
		return true;
	}
	return false;
}

void addKobjectType(StructType *type, int index) {
	auto it = kobjectTypes.find(type);
	if (it == kobjectTypes.end()) {
		cout << "kobjectType: " << type->getName().data() << endl;
		kobjectTypes.insert(make_pair(type, index));
	}
}

void addObjbindStruct(Module *module, StructType *ty, int fidx) {
	auto objbind_map = objbindStructs.find(ty);
	if (objbind_map == objbindStructs.end()) {
		uint64_t offset = getFieldOffsetInByte(module, ty, fidx);
		ObjbindAttr attr;
		attr.candidateFields.insert(make_pair(fidx, offset));
		objbindStructs.insert(make_pair(ty, attr));
	} else {
		// compute element offset in byte
		uint64_t offset = getFieldOffsetInByte(module, ty, fidx);
		objbind_map->second.candidateFields.insert(make_pair(fidx, offset));
	}
}

void collectObjbindStructs(Module *module, char *structName, bool pointerOnly) {
	llvm::TypeFinder structTypes;

	structTypes.run(*module, true);

	for (auto *ty: structTypes) {
		bool tflag = false;

		// [JINBUM]
		//if (structName && strncmp(ty->getName().data(), structName, strlen(structName)) == 0)
		if (structName && strcmp(ty->getName().data(), structName) == 0) // not include struct.aaa.*
		{
			tflag = true;
		} else if (structName) {
			continue;
		}

		bool fptr = false;

		// check a speical kobject type (struct.kobject for Linux, ? for FreeBSD)
		if (isKobjectType(ty)) {
			continue;
		}

		// consider the result of statistics
		if (sortedTypesForObjbind.size() > 0 && (sortedTypesForObjbind.find(ty) == sortedTypesForObjbind.end())) {
			continue;
		}

		for (auto element: ty->elements()) {
			Type *type = &*element;
			
			if (type->isStructTy() == false && isFunctionPointerType(type)) {
				fptr = true;
				break;
			}
		}

		if (fptr) {
			int fidx = 0;

			for (auto element: ty->elements()) {
				Type *type = &*element;

				if (type->isStructTy() == false && isFunctionPointerType(type) == false) {
					if ((pointerOnly && type->isPointerTy()) || !pointerOnly) {
						addObjbindStruct(module, ty, fidx);
					}
				} else if (type->isStructTy()) {
					StructType *ST = dyn_cast<StructType>(type);
					if (isKobjectType(ST)) {
						addKobjectType(ty, fidx);
					} else {
						// try to find in one more depth
						for (auto el: ST->elements()) {
							Type *type = &*el;
							if (type->isStructTy()) {
								StructType *st = dyn_cast<StructType>(type);
								if (st) {
									if (isKobjectType(st)) {
										addKobjectType(ty, fidx);
										break;
									}
								}
							}
							else if ((pointerOnly && type->isPointerTy()) || !pointerOnly) {
								addObjbindStruct(module, ty, fidx);
								break;
							}
						}
					}
				}

				fidx += 1;
			}
		}
	}

	if (objbindStructs.size() % 50 == 0) {
		cout << "number of objbindStructs: " << objbindStructs.size() << endl;
		cout << "number of kobjectTypes: " << kobjectTypes.size() << endl;
	}
}

GetElementPtrInst *getGEP(Value *V) {
	if (V == NULL)
		return NULL;
	if (auto GEP = dyn_cast_or_null<GetElementPtrInst>(V)) {
		return GEP;
	} else if (auto BCI = dyn_cast_or_null<BitCastInst>(V)) {
		Value *v2 = BCI->getOperand(0);
		if (auto GEP = dyn_cast_or_null<GetElementPtrInst>(v2)) {
			return GEP;
		}
	}
	return NULL;
}

void addObjbindWorklist(vector<ObjbindFirstRoundAttr> &worklist, StructType *type, Function *func, StoreInst *insn, unsigned int fieldIdx, uint64_t fieldOffset) {
	ObjbindFirstRoundAttr attr;

	attr.fieldIdx = fieldIdx;
	attr.fieldOffset = fieldOffset;
	attr.insn = insn;
	attr.func = func;
	attr.type = type;
	attr.arg = NULL;
	attr.argIdx = 0;

	worklist.push_back(attr);
}

//#define DEBUG_RESOLVE 1

bool resolveValueForObjbind(StructType *type, Function *func, Function *targetFunc, Value *val, int fieldIdx, Argument **argOut, int *argIdxOut) {
	Constant *constant = dyn_cast<Constant>(val);
	CallInst *call = dyn_cast<CallInst>(val);
	AllocaInst *alloca = dyn_cast<AllocaInst>(val);
	BitCastInst *bci = dyn_cast<BitCastInst>(val);
	LoadInst *load = dyn_cast<LoadInst>(val);
	Argument *argm = dyn_cast<Argument>(val);
	GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(val);

	auto structIt = objbindStructs.find(type);
	if (structIt == objbindStructs.end())
		return false;

	if (bci) {
		Value *v = bci->getOperand(0);
		load = dyn_cast<LoadInst>(v);
		alloca = dyn_cast<AllocaInst>(v);
		constant = dyn_cast<Constant>(v);
		call = dyn_cast<CallInst>(v);
		argm = dyn_cast<Argument>(v);
		gep = dyn_cast<GetElementPtrInst>(v);
	}
	if (load) {
		Value *v = load->getPointerOperand();
		constant = dyn_cast<Constant>(v);
		alloca = dyn_cast<AllocaInst>(v);
		call = dyn_cast<CallInst>(v);
		argm = dyn_cast<Argument>(v);
		gep = dyn_cast<GetElementPtrInst>(v);
	}
	if (gep) {
		Value *v = gep->getPointerOperand();
		constant = dyn_cast<Constant>(v);
		alloca = dyn_cast<AllocaInst>(v);
		call = dyn_cast<CallInst>(v);
		argm = dyn_cast<Argument>(v);
	}

	// (1) If it is constant, increments a score (+ solving load one-depth further)
	if (constant) {
		DiversityScore &score = structIt->second.score[fieldIdx];
		auto ret = score.value.insert(val);
		if (ret.second == true)
			score.score += 1;
		//score.score += 1;  // [TODO] consider redundancy

		#ifdef DEBUG_RESOLVE
		// print for debugging
		string str;
		raw_string_ostream output(str);

		constant->print(output);
		cout << "===== CONSTANT ====" << endl;
		cout << "Current-Function: " << func->getName().data() << endl;
		//if (targetFunc)
		//	cout << "\t" << "Target-Function: " << targetFunc->getName().data() << endl;
		cout << "\t" << "Constant: " << str << endl;
		cout << "\t" << "Struct: " << type->getName().data() << endl;
		cout << "\t" << "FieldIdx: " << fieldIdx << endl;
		cout << endl;
		#endif

		return false;
	}
	// (2) If it is a function call, non-measurable; [TODO] it can be refined
	else if (call) {
		Function *func = call->getCalledFunction();
		DiversityScore &score = structIt->second.score[fieldIdx];

		#ifdef DEBUG_RESOLVE
		if (func) {
			cout << " ==== CALL ====" << endl;
			cout << "\t" << "CalledFunction: " << func->getName().data() << endl;
			cout << "\t" << "Struct: " << type->getName().data() << endl;
			cout << "\t" << "FieldIdx: " << fieldIdx << endl;
		}
		#endif

		// check heap allocators (TODO: incomplete yet)
		if (func) {
			if (strstr(func->getName().data(), "kmem_cache") != NULL ||
				strstr(func->getName().data(), "alloc") != NULL) {
				score.score += 1;

				/*
				if (func) {
					cout << " ==== SCORE-ALLOCATOR-CALL ====" << endl;
					cout << "\t" << "CalledFunction: " << func->getName().data() << endl;
					cout << "\t" << "Struct: " << type->getName().data() << endl;
					cout << "\t" << "FieldIdx: " << fieldIdx << endl;
				} */
			}
		} else {
			score.nonMeasurableScore += 1;
		}

		return false;
	}
	// (3) Special case-1: If it points to a stack address (one-time fuction pointer), treat it as Constant.
	else if (alloca) {
		AllocaInst *insn = alloca;

		if (insn) { // if it points to a stack address
			DiversityScore &score = structIt->second.score[fieldIdx];
			auto ret = score.value.insert(val);
			if (ret.second == true)
				score.score += 1;

			#ifdef DEBUG_RESOLVE
			// print for debugging
			string str;
			raw_string_ostream output(str);

			insn->print(output);
			cout << "===== ALLOCA-CONSTANT ====" << endl;
			cout << "Current-Function: " << func->getName().data() << endl;
			//if (targetFunc)
			//	cout << "\t" << "Target-Function: " << targetFunc->getName().data() << endl;
			cout << "\t" << "ALLOCA: " << str << endl;
			cout << "\t" << "Struct: " << type->getName().data() << endl;
			cout << "\t" << "FieldIdx: " << fieldIdx << endl;
			cout << endl;
			#endif
			
			return false;
		}
	}
	// (4) GEP --> track a struct type of GEP, and score if this is a new struct type
	else if (gep) {
		StructType *st = getBaseStructPtrType(gep);
		if (st) {
			DiversityScore &score = structIt->second.score[fieldIdx];
			auto ret = score.types.insert(st);
			if (ret.second == true)
				score.score += 1;
			
			#ifdef DEBUG_RESOLVE
			cout << "===== GEP-STRUCT ====" << endl;
			cout << "Current-Function: " << func->getName().data() << endl;
			cout << "\t" << "Value-Struct: " << st->getName().data() << endl;
			cout << "\t" << "Struct: " << type->getName().data() << endl;
			cout << "\t" << "FieldIdx: " << fieldIdx << endl;
			cout << endl;
			#endif
		}
		return false;
	}
	// (5) Otherwise, check arguments
	else {
		int idx = 0;
		for (Function::arg_iterator ait = func->arg_begin(), aite = func->arg_end(); ait != aite; ++ait) {
			Argument *arg = &*ait;
			Value *argv = dyn_cast<Value>(arg);

			// If it needs to be further resolved, returns true;
			if (argm != NULL && argv == val) {
				*argOut = arg;
				*argIdxOut = idx;
				return true;
			}

			idx += 1;
		}

		DiversityScore &score = structIt->second.score[fieldIdx];
		score.undefScore += 1;

		#ifdef DEBUG_RESOLVE
		// print for debugging
		string str, funcStr;
		raw_string_ostream output(str), outputFunc(funcStr);

		val->print(output);
		cout << "===== Non-Constant + Non-Argument ====" << endl;
		cout << "Current-Function: " << func->getName().data() << endl;
		//if (targetFunc)
		//	cout << "\t" << "Target-Function: " << targetFunc->getName().data() << endl;
		cout << "\t" << "Val: " << str << endl;
		cout << "\t" << "Struct: " << type->getName().data() << endl;
		cout << "\t" << "FieldIdx: " << fieldIdx << endl;
		/* [test]
		if (load) {
			string lstr;
			raw_string_ostream ostr(lstr);
			load->print(ostr);
			cout << lstr << endl;
		} else if (bci) {
			Value *v = bci->getOperand(0);
			string lstr;
			raw_string_ostream ostr(lstr);
			v->print(ostr);
			cout << lstr << endl;

			auto GEP = dyn_cast<GetElementPtrInst>(v);
			if (GEP) {
				Value *v = GEP->getPointerOperand();
				string lstr;
				raw_string_ostream ostr(lstr);
				v->print(ostr);
				cout << lstr << endl;
			}
		} */
		cout << endl;
		#endif

		//func->print(outputFunc);
		//cout << funcStr << endl;
	}

	return false;
}

void resolveObjbindViaWorklist(Module *module, ObjbindFirstRoundAttr &first) {
	// worklist algorithm
	map<Function *, ObjbindWorklistAttr> worklist;
	map<Function *, ObjbindWorklistAttr> updated;

	int threshold_depth = 3;
	int depth = 0;

	// 1. set initial worklist
	ObjbindWorklistAttr attr;
	attr.type = first.type;
	attr.arg = first.arg;
	attr.argIdx = first.argIdx;
	attr.fieldIdx = first.fieldIdx;
	attr.fieldOffset = first.fieldOffset;
	worklist.insert(make_pair(first.func, attr));

	// 2. worklist main loop
	while (depth < threshold_depth && worklist.size() > 0) {
		// 3. collect next round functions in updated
		for (auto it: worklist) {
			Function *targetFunc = it.first;
			ObjbindWorklistAttr &worklistAttr = it.second;

			for (auto curFunc = module->getFunctionList().begin(), 
				endFunc = module->getFunctionList().end();
				curFunc != endFunc; ++curFunc) {
				for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
					bool end = false;

					for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
						// scan all instructions
						Instruction *I = &*BI++;
						CallInst *call = dyn_cast<CallInst>(I);

						if (call) {
							Function *func = call->getCalledFunction();
							if (func && func == targetFunc) {
								// try to resolve the function
								if (worklistAttr.argIdx < call->getNumArgOperands()) {
									Value *arg = call->getArgOperand(worklistAttr.argIdx);

									Argument *argOut = NULL;
									int argIdxOut = 0;

									if (resolveValueForObjbind(first.type, &*curFunc, targetFunc, arg, first.fieldIdx, &argOut, &argIdxOut) == true) {
										ObjbindWorklistAttr newAttr;
										newAttr.type = first.type;
										newAttr.arg = argOut;
										newAttr.argIdx = argIdxOut;
										newAttr.fieldIdx = first.fieldIdx;
										newAttr.fieldOffset = first.fieldOffset;

										updated.insert(make_pair(&*curFunc, newAttr));
										end = true;
										break;
									}
								}
							}
						}
					}

					if (end)
						break;
				}
			}
		}

		// 4. update worklist, and prepare next round
		worklist.clear();
		worklist = updated;
		updated.clear();
		depth += 1;
	}
}

void objbindIncrementConstantScore(StructType *type, Value *val, int fieldIdx) {
	auto structIt = objbindStructs.find(type);
	if (structIt == objbindStructs.end())
		return;
	
	DiversityScore &score = structIt->second.score[fieldIdx];
	auto ret = score.value.insert(val);
	if (ret.second == true)
		score.score += 1;
}

void objbindIncrementGlobalAddrBindScore(StructType *type) {
	auto structIt = objbindStructs.find(type);
	if (structIt == objbindStructs.end())
		return;
	
	auto globalIt = objbindGlobalAddrBindScores.find(type);
	if (globalIt == objbindGlobalAddrBindScores.end()) {
		objbindGlobalAddrBindScores.insert(make_pair(type, 1));
	} else {
		globalIt->second += 1;
	}
}

void diversityScoreForConstantStruct(Constant *con, StructType *type, unsigned int fieldIdx, bool globalAddrBind) {
	StructType *st = dyn_cast<StructType>(con->getType());
	if (st == type) {
		if (globalAddrBind) {
			objbindIncrementGlobalAddrBindScore(type);
			return;
		}
		unsigned num = st->getNumElements();
		if (fieldIdx < num) {
			Constant *c = con->getAggregateElement(fieldIdx);
			Value *v = dyn_cast<Value>(c);
			Type *t = v->getType();

			if (t->isStructTy()) {  // If struct, look at in one more depth
				StructType *innerType = dyn_cast<StructType>(t);
				for (unsigned i = 0; i < innerType->getNumElements(); i++) {
					Constant *innerC = c->getAggregateElement(i);
					Value *innerV = dyn_cast<Value>(innerC);
					objbindIncrementConstantScore(type, innerV, fieldIdx);
				}
			} else {
				objbindIncrementConstantScore(type, v, fieldIdx);
			}
		}
	} /* else if (st) {
		// st == platform_driver
		// type == device_driver
		// look one more depth
		unsigned num = st->getNumElements();
		for (unsigned i = 0; i < num; i++) {
			Constant *c = con->getAggregateElement(i);
			Value *v = dyn_cast<Value>(c);
			Type *t = v->getType();  // device_driver

			if (t->isStructTy()) {  // If struct, look at in one more depth
				StructType *innerType = dyn_cast<StructType>(t);
				if (innerType == type) {
					if (globalAddrBind) {
						objbindIncrementGlobalAddrBindScore(type);
						return;
					}

					if (fieldIdx < innerType->getNumElements()) {
						Constant *innerC = c->getAggregateElement(fieldIdx);
						Value *innerV = dyn_cast<Value>(innerC);
						objbindIncrementConstantScore(type, innerV, fieldIdx);
					}
				}
			}
		}
	} */
}

void diversityScoreForGlobalVariables(Module *module, StructType *type, unsigned int fieldIdx, uint64_t fieldOffset, bool globalAddrBind) {
	for (auto gv_iter = module->global_begin(); gv_iter != module->global_end(); gv_iter++) {
		GlobalVariable *gv = &*gv_iter;

		if (gv->hasInitializer()) {
			const ConstantStruct *cs = dyn_cast<ConstantStruct>(gv->getInitializer());
			const ConstantArray *arr = dyn_cast<ConstantArray>(gv->getInitializer());

			if (cs) {
				diversityScoreForConstantStruct((Constant *)cs, type, fieldIdx, globalAddrBind);
			} else if (arr) {
				ArrayType *at = arr->getType();
				Type *elt = at->getElementType();
				unsigned num = at->getNumElements();

				for (unsigned i = 0; i < num; i++) {
					Constant *c = arr->getAggregateElement(i);
					diversityScoreForConstantStruct(c, type, fieldIdx, globalAddrBind);
				}
			}
		}
	}
}

void calculateObjbindDiversityScore(Module *module, StructType *type, unsigned int fieldIdx, uint64_t fieldOffset) {
	
	vector<ObjbindFirstRoundAttr> firstRoundStore;

	auto structIt = objbindStructs.find(type);

	firstRoundStore.clear();

	// =============================================================================================
	// 0. Perform on global variables first
	diversityScoreForGlobalVariables(module, type, fieldIdx, fieldOffset, false);

	// =============================================================================================
	// 1. Initial Round
	// Run initial round to set a first worklist
	for (auto curFunc = module->getFunctionList().begin(), 
				endFunc = module->getFunctionList().end();
				curFunc != endFunc; ++curFunc) {
		// tracking bitcasting operands first
		set<Value *> bciAlias; // Aliased instruction to type through BCI instruction

		// (1) resolve bci aliasing
		for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
			for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
				Instruction *I = &*BI++;
				BitCastInst *BCI = dyn_cast<BitCastInst>(I);

				if (BCI) {
					Value *src = BCI->getOperand(0);
					Type *dest = BCI->getDestTy();
					StructType *ST = NULL;

					if (src == NULL || dest == NULL)
						continue;

					if (dest->isPointerTy()) {
						dest = dest->getPointerElementType();
						ST = dyn_cast<StructType>(dest);
					}

					if (ST == type) {
						// [test]
						/*
						string s1, s2, s3;
						raw_string_ostream o1(s1), o2(s2), o3(s3);
						src->print(o1);
						BCI->print(o2);
						cout << endl;
						cout << "BCI: " << s2 << endl;
						cout << "BCI-SRC: " << s1 << endl;
						
						auto GEP = dyn_cast<GetElementPtrInst>(src);
						if (GEP) {
							GEP->getPointerOperand()->print(o3);
							cout << "BCI-SRC-GEP: " << s3 << endl;
						}
						cout << "ST: " << ST->getName().data() << endl << endl; */

						bciAlias.insert(src);
					}
				}
			}
		}

		// (2) find store instructions
		for (Function::iterator BB = curFunc->begin(), BBE = curFunc->end(); BB != BBE; ++BB) {
			for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
				Instruction *I = &*BI++;
				StoreInst *SI = dyn_cast<StoreInst>(I);

				if (SI) {
					Value *V = SI->getPointerOperand();
					StructType *ST = getBaseStructPtrType(V);

					// Case-1: GEP on the struct type pointer
					if (ST == type) {
						auto GEP = getGEP(V);

						if (GEP) {
							unsigned idx = GEP->getNumIndices();
							Value *GV = GEP->getOperand(idx);
							if (GV) {
								ConstantInt *CI = dyn_cast<ConstantInt>(GV);
								if (CI) {
									uint64_t index = CI->getZExtValue();

									if (index == (uint64_t)fieldIdx) {
										addObjbindWorklist(firstRoundStore, type, &*curFunc, SI, fieldIdx, fieldOffset);
									}
								}
							}
						}
					}
					// Case-2: GEP on casted pointer to the struct type (use bciAlias)
					else {
						Value *GEPV = getGEPPointerOperand(V);
						if (GEPV) {
							auto it = bciAlias.find(GEPV);
							if (it != bciAlias.end()) {
								// [TODO] i8 type check
								auto GEP = getGEP(V);

								if (GEP) {
									unsigned idx = GEP->getNumIndices();
									Value *GV = GEP->getOperand(idx);
									if (GV) {
										ConstantInt *CI = dyn_cast<ConstantInt>(GV);
										if (CI) {
											uint64_t gepOffset = CI->getZExtValue();

											if (gepOffset == fieldOffset) {
												addObjbindWorklist(firstRoundStore, type, &*curFunc, SI, fieldIdx, fieldOffset);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	// =============================================================================================

	// =============================================================================================
	// 2. Resolving StoreInst through worklist algorithm (whether it is constant or not)
	//    -- If the source value of the StoreInst is constant or read-only symbol (e.g., function symbol), it is treated as constant.
	//       Further resolving is not required.
	//    -- Otherwise, resolving (tracking) for pointer-carrying functions.

	for (auto attr: firstRoundStore) {
		StoreInst *insn = attr.insn;
		Value *val = insn->getValueOperand();
		Constant *constant = dyn_cast<Constant>(val);
		CallInst *call = dyn_cast<CallInst>(val);

		Argument *arg = NULL;
		int argIdx = 0;

		// [test]
		/*
		string s1, s2;
		raw_string_ostream o1(s1), o2(s2);
		val->print(o1);
		insn->print(o2);
		cout << "SI: " << s2 << endl;
		cout << "SI-val: " << s1 << endl; */

		if (resolveValueForObjbind(type, attr.func, NULL, val, fieldIdx, &arg, &argIdx) == true) {
			attr.arg = arg;
			attr.argIdx = argIdx;

			// Start worklist algorithm
			resolveObjbindViaWorklist(module, attr);
			break;
		}
	}
}

void doObjbindAnalysis(Module *module) {
	unsigned i = 0;
	for (auto it: objbindStructs) {
		for (auto field: it.second.candidateFields) {
			calculateObjbindDiversityScore(module, it.first, field.first, field.second);
		}
		i++;
		if (i % 10 == 0)
			cout << "[OBJBIND][RESOLVE] Progress: " << i << " / " << objbindStructs.size() << endl;
	}

	// for GlobalAddrBindScore
	i = 0;
	for (auto it: objbindStructs) {
		diversityScoreForGlobalVariables(module, it.first, 0, 0, true);
		i++;
		if (i % 10 == 0)
			cout << "[OBJBIND][GLOBAL] Progress: " << i << " / " << objbindStructs.size() << endl;
	}
}

typedef struct __ObjbindVal {
	int maxFieldIdx;
	unsigned int maxScore;
} ObjbindVal;

void printObjbindResult(Module *module) {
	FILE *fp = NULL;
	fp = fopen("objbind_score.csv", "w");

	cout << "===== Objbind Analysis Result =====" << endl;

	map<string, ObjbindVal> resultMap;

	for (auto it: objbindStructs) {
		Type *type = it.first;
		StructType *ST = dyn_cast<StructType>(type);
		unsigned int maxScore = 0;
		int maxFieldIdx = -1;

		if (ST) {
			auto kobject = kobjectTypes.find(ST);

			cout << endl;
			cout << "==== Struct: " << ST->getName().data() << " ====" << endl;
			cout << "Candidate fields: ";
			for (auto field: it.second.candidateFields) {
				cout << field.first << ", ";
			}
			cout << endl;
			if (kobject != kobjectTypes.end()) {
				cout << "Kobject field: " << kobject->second << endl;
				
				if (maxScore < 1000) {
					maxScore = 1000;
					maxFieldIdx = kobject->second;
				}
			}

			for (auto field: it.second.score) {
				cout << "Field-" << field.first << ": " << endl;
				cout << "\t" << "DiversityScore: " << field.second.score << endl;
				cout << "\t" << "Non-Measurable Score: " << field.second.nonMeasurableScore << endl;
				cout << "\t" << "Undefined Score: " << field.second.undefScore << endl;

				if (maxScore < field.second.score) {
					maxScore = field.second.score;
					maxFieldIdx = field.first;
				}
			}

			auto globalIt = objbindGlobalAddrBindScores.find(ST);
			if (globalIt != objbindGlobalAddrBindScores.end()) {
				cout << "GlobalAddrBindScore: " << globalIt->second << endl;

				if (maxScore < globalIt->second) {
					maxScore = globalIt->second;
					maxFieldIdx = -2;  // special value for GlobalAddrBind
				}
			}

			cout << endl;

			stringstream ss(ST->getName());
			string token;
			string name = "";

			getline(ss, token, '.');
			name += token;

			getline(ss, token, '.');
			name += ".";
			name += token;

			ObjbindVal val;
			val.maxFieldIdx = maxFieldIdx;
			val.maxScore = maxScore;
			resultMap.insert(make_pair(name, val));

			// Write result
			/*
			if (fp) {
				stringstream ss(ST->getName());
				string token;
				string name = "";

				getline(ss, token, '.');
				name += token;

				getline(ss, token, '.');
				name += ".";
				name += token;

				char str[512] = {0,};
				sprintf(str, "%s,%d,%d\n", name.data(), maxFieldIdx, maxScore);
				fwrite(str, strlen(str), 1, fp);
			} */
		}
	}

	// sort
	vector<pair<string, ObjbindVal>> sortedResult(resultMap.begin(), resultMap.end());
	std::sort(sortedResult.begin(), sortedResult.end(), [](pair<string, ObjbindVal> a, pair<string, ObjbindVal> b) {
		return (a.second.maxScore) > (b.second.maxScore);
	});

	// file write
	for (auto it: sortedResult) {
		if (fp) {
			char str[512] = {0,};
			sprintf(str, "%s,%d,%d\n", it.first.data(), it.second.maxFieldIdx, it.second.maxScore);
			fwrite(str, strlen(str), 1, fp);
		}
	}

	if (fp)
		fclose(fp);
}

void objbindAnalysis(Module *module, char *structName, char *inputFileName, bool pointerOnly) {
	// initialize
	objbindStructs.clear();

	// analysis
	if (structName) {
		collectObjbindStructs(module, structName, pointerOnly);
	} else if (structName == NULL && inputFileName) {
		string line;
		fstream fs;

		fs.open(inputFileName, fstream::in);
		while(getline(fs, line)) {
			collectObjbindStructs(module, (char *)line.data(), pointerOnly);
		}
		fs.close();
	} else {
		collectObjbindStructs(module, NULL, pointerOnly);
	}
	doObjbindAnalysis(module);

	// reporting
	printObjbindResult(module);
}
// ======================================================

int main(int argc, char ** argv) {
	if (argc < 4) {
		std::cout << "USAGE: ./analyzer [bitcode file name] [objbind or retbind or stat] [pointer-only-flag or retbind threshold] [objbind struct name]" << std::endl;
		std::cout << std::endl;
		std::cout << "objbind (pointer-only) for a certain struct: ./analyzer vmlinux.bc objbind 1 struct.irqaction" << std::endl;
		std::cout << "objbind (all-field) for a certain struct: ./analyzer vmlinux.bc objbind 0 struct.irqaction" << std::endl;
		std::cout << "objbind for all structs: ./analyzer vmlinux.bc objbind 1" << std::endl;
		std::cout << "objbind for specified structs: ./analyzer vmlinux.bc objbind 1 objbind_structs.csv" << std::endl;
		std::cout << "--- pointer-only mode performs static analysis only for pointer-type fields ---" << std::endl;
		std::cout << "For retbind: ./analyzer vmlinux.bc retbind 20" << std::endl;
		std::cout << "For retbind: ./analyzer vmlinux.bc retbind 20 retbind_functions.csv" << std::endl;
		std::cout << "--- here, 20 is a threshold value on the number of callers ---" << std::endl;
		std::cout << "stat: ./analyzer vmlinux.bc stat 100" << std::endl;
		std::cout << "--- 'stat' computes and reports allowed targets ---" << std::endl;
		return -1;
	}

	LLVMContext &Context = getGlobalContext();
	char *objbindStruct = NULL;
	bool objbindListFromFile = false;

	if (argc == 5) {
		if (strstr(argv[4], "struct.") != NULL)
			objbindStruct = argv[4];
		else
			objbindListFromFile = true;
	}

	std::string InputFilename(argv[1]);
	std::string Command(argv[2]);
	bool pointer_only = false;
	int threshold = 0;
	int thresholdAllowedTarget = 100;

	if (Command == "objbind") {
		pointer_only = (atoi(argv[3]) == 0) ? false : true;
	} else if (Command == "retbind") {
		threshold = atoi(argv[3]);
	} else if (Command == "stat") {
		thresholdAllowedTarget = atoi(argv[3]);
	}
	
	SMDiagnostic Err;

	cout << " ============================================================" << endl;
	std::cout << "start: parseIRFile: " << argv[1] << std::endl;
	std::unique_ptr<Module> M1 = parseIRFile(InputFilename, Err, Context);
	std::cout << "end: parseIRFile: " << argv[1] << std::endl;
	cout << " ============================================================" << endl;

	if (!M1) {
		Err.print(argv[0], errs());
		return 1;
	}

	cout << " ============================================================" << endl;
	sortedTypesForObjbind.clear();
	cout << " ============================================================" << endl;

	if (Command == "objbind") {
		cout << " ============================================================" << endl;
		if (objbindListFromFile == false) {
			objbindAnalysis(M1.get(), objbindStruct, NULL, pointer_only);
		} else {
			objbindAnalysis(M1.get(), NULL, argv[4], pointer_only);
		}
		cout << " ============================================================" << endl;
	} else if (Command == "retbind") {
		retbindAnalysis(M1.get(), threshold);
	} else if (Command == "stat") {
		doStatistics(M1.get(), thresholdAllowedTarget, true);
	}

	return 0;
}

