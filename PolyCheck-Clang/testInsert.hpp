#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Lex/Pragma.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>

#include <map>
#include <set>

using namespace clang;
using namespace std;

CompilerInstance* pTheCompInst_g;

int ParseScop(string fileName, string prolog, string epilog, string outputFileName);

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T>& vec) {
    os << "[";
    for(auto& x : vec) os << x << ",";
    os << "]\n";
    return os;
}

string GetOutputFileName(string fileName)
{
  string outputFileName;
  
  int last_index = fileName.length() - 1;
  int begin_index = 0;
  
  for (int i = last_index; i >= 0; i--)
  {
    if (fileName[i] == '/')
    {
      begin_index = i+1;
      break;
    }// if (fileName[i] == '/')
  }// for (int i = last_index; i >= 0; i--)
  
  if (begin_index > last_index)
  {
     assert("File name is not correct");
  }// if (begin_index > last_index)
  
  outputFileName = "pc_" + fileName.substr(begin_index, last_index - begin_index + 1);
 
  return outputFileName;
}// string Processor::GetOutputFileName(char *fileName)

namespace islw {

////////////////////////////////////////////////////////////////////////////////

template<typename T>
isl_ctx* context(T* obj);

template<>
isl_ctx* context(isl_union_map* umap) {
    return isl_union_map_get_ctx(umap);
}

template<>
isl_ctx* context(isl_map* map) {
    return isl_map_get_ctx(map);
}

template<>
isl_ctx* context(isl_ast_node* node) {
    return isl_ast_node_get_ctx(node);
}

template<>
isl_ctx* context(isl_point* point) {
    return isl_point_get_ctx(point);
}

template<>
isl_ctx* context(isl_set* iset) {
    return isl_set_get_ctx(iset);
}

template<>
isl_ctx* context(isl_union_set* uset) {
    return isl_union_set_get_ctx(uset);
}

template<>
isl_ctx* context(isl_qpolynomial* qp) {
    return isl_qpolynomial_get_ctx(qp);
}

template<>
isl_ctx* context(isl_id* id) {
    return isl_id_get_ctx(id);
}

template<>
isl_ctx* context(isl_space* obj) {
    return isl_space_get_ctx(obj);
}

template<>
isl_ctx* context(isl_multi_pw_aff* mpwa) {
    return isl_multi_pw_aff_get_ctx(mpwa);
}

template<>
isl_ctx* context(isl_union_pw_qpolynomial* obj) {
    return isl_union_pw_qpolynomial_get_ctx(obj);
}

template<>
isl_ctx* context(isl_pw_qpolynomial* obj) {
    return isl_pw_qpolynomial_get_ctx(obj);
}

template<>
isl_ctx* context(isl_schedule* obj) {
    return isl_schedule_get_ctx(obj);
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
isl_printer* to_printer(isl_printer* printer, T* obj);

template<>
isl_printer* to_printer(isl_printer* printer, isl_union_map* obj) {
    return isl_printer_print_union_map(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_map* obj) {
    return isl_printer_print_map(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_schedule* obj) {
    return isl_printer_print_schedule(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_set* obj) {
    return isl_printer_print_set(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_union_set* obj) {
    return isl_printer_print_union_set(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_point* point) {
    return isl_printer_print_point(printer, point);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_space* obj) {
    return isl_printer_print_space(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_id* id) {
    return isl_printer_print_id(printer, id);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_ast_node* node) {
    return isl_printer_print_ast_node(printer, node);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_qpolynomial* obj) {
    return isl_printer_print_qpolynomial(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_union_pw_qpolynomial* obj) {
    return isl_printer_print_union_pw_qpolynomial(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_pw_qpolynomial* obj) {
    return isl_printer_print_pw_qpolynomial(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_multi_pw_aff* obj) {
    return isl_printer_print_multi_pw_aff(printer, obj);
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
void destruct(T* obj);

template<>
void destruct(isl_id* id) {
    if(id) {
        isl_id_free(id);
    }
}

template<>
void destruct(isl_point* point) {
    if(point) {
        isl_point_free(point);
    }
}

template<>
void destruct(isl_union_set* uset) {
    if(uset) {
        isl_union_set_free(uset);
    }
}

template<>
void destruct(isl_union_map* umap) {
    if(umap) {
        isl_union_map_free(umap);
    }
}

template<>
void destruct(isl_map* map) {
    if(map) {
        isl_map_free(map);
    }
}

template<>
void destruct(isl_ctx* ctx) {
    if(ctx) {
        isl_ctx_free(ctx);
    }
}

template<>
void destruct(isl_printer* prt) {
    if(prt) {
        isl_printer_free(prt);
    }
}

template<>
void destruct(isl_ast_node* node) {
    if(node) {
        isl_ast_node_free(node);
    }
}

template<>
void destruct(isl_set* iset) {
    if(iset) {
        isl_set_free(iset);
    }
}

template<>
void destruct(isl_ast_build* build) {
    if(build) {
        isl_ast_build_free(build);
    }
}

template<>
void destruct(isl_pw_qpolynomial* upwqp) {
    if(upwqp) {
        isl_pw_qpolynomial_free(upwqp);
    }
}

template<>
void destruct(isl_union_pw_qpolynomial* pwqp) {
    if(pwqp) {
        isl_union_pw_qpolynomial_free(pwqp);
    }
}

template<>
void destruct(isl_qpolynomial* qp) {
    if(qp) {
        isl_qpolynomial_free(qp);
    }
}

template<>
void destruct(isl_schedule* sc) {
    if(sc) {
        isl_schedule_free(sc);
    }
}

template<typename T, typename... Args>
void destruct(T* obj, Args... args) {
    islw::destruct(obj);
    islw::destruct(std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
T* copy(T* obj);

template<>
isl_union_map* copy(isl_union_map* umap) {
    return umap ? isl_union_map_copy(umap) : nullptr;
}

template<>
isl_map* copy(isl_map* map) {
    return map ? isl_map_copy(map) : nullptr;
}

template<>
isl_space* copy(isl_space* space) {
    return space ? isl_space_copy(space) : nullptr;
}

template<>
isl_union_pw_qpolynomial* copy(isl_union_pw_qpolynomial* obj) {
    return obj ? isl_union_pw_qpolynomial_copy(obj) : nullptr;
}

template<>
isl_union_set* copy(isl_union_set* uset) {
    return uset ? isl_union_set_copy(uset) : nullptr;
}

template<>
isl_set* copy(isl_set* iset) {
    return iset ? isl_set_copy(iset) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
std::string to_string(T* obj, int output_format = ISL_FORMAT_ISL) {
    isl_ctx* ctx = context(obj);
    isl_printer* printer = isl_printer_to_str(ctx);
    printer = isl_printer_set_output_format(printer, output_format);
    printer = to_printer(printer, obj);
    char* output = isl_printer_get_str(printer);
    isl_printer_free(printer);
    std::string ret{output};
    free(output);
    return ret;
}

// template<>
// std::string to_string(isl_id* id, int output_format) {
//     return std::string{isl_id_get_name(id)};
// }

template<typename T>
std::string to_c_string(T* obj) {
    return to_string(obj, ISL_FORMAT_C);
}

template<>
std::string to_c_string(isl_union_map* umap) {
    isl_set* cset        = isl_set_universe(isl_union_map_get_space(umap));
    isl_ast_build* build = isl_ast_build_from_context(cset);
    isl_ast_node* node =
      isl_ast_build_ast_from_schedule(build, islw::copy(umap));
    std::string str = islw::to_c_string(node);
    islw::destruct(build);
    islw::destruct(node);
    return str;
}

template<>
std::string to_c_string(isl_set* iset) {
    isl_union_map* umap = isl_union_set_identity(
        isl_union_set_from_set(islw::copy(iset)));
    std::string str =  islw::to_c_string(umap);
    islw::destruct(umap);
    return str;
}

////////////////////////////////////////////////////////////////////////////////

isl_union_set* apply_map(isl_union_map* umap, isl_union_set* uset) {
    return isl_union_map_range(
      isl_union_map_intersect_domain(islw::copy(umap), islw::copy(uset)));
}

isl_union_map* umap_compose_left(isl_union_map* umap1, isl_union_map* umap2) {
    return isl_union_map_apply_range(umap1, copy(umap2));
}

isl_union_map* umap_compose(isl_union_map* umap1, isl_union_map* umap2) {
    return isl_union_map_apply_range(copy(umap1), copy(umap2));
}

template<typename... Args>
isl_union_map* umap_compose_left(isl_union_map* umap1, isl_union_map* umap2, Args... args) {
    isl_union_map* ret = umap_compose(umap1, copy(umap2));
    return umap_compose_left(ret, args...);
}

template<typename... Args>
isl_union_map* umap_compose(isl_union_map* umap1, isl_union_map* umap2, Args... args) {
    isl_union_map* ret = umap_compose(umap1, umap2);
    return umap_compose_left(ret, args...);
}


////////////////////////////////////////////////////////////////////////////////

} // namespace islw


isl_stat print_pw_qpoly(__isl_take isl_pw_qpolynomial *pwqp, void *user) {
    assert(pwqp);
    assert(user);
    std::string& str = *(std::string*)user;
    std::cout<<"pwqp in c:---\n"<<islw::to_c_string(pwqp)<<"\n---\n";
    str += islw::to_c_string(pwqp);
    return isl_stat_ok;
}

void GatherStmtOpIds(vector<string>& stmtOpIds, struct pet_expr* expr){
  if(pet_expr_op_get_type(expr) == pet_op_add_assign)
    stmtOpIds.push_back(string("+="));
  else if(pet_expr_op_get_type(expr) == pet_op_sub_assign)
    stmtOpIds.push_back(string("-="));
  else if(pet_expr_op_get_type(expr) == pet_op_mul_assign)
    stmtOpIds.push_back(string("*="));
  else if(pet_expr_op_get_type(expr) == pet_op_div_assign)
    stmtOpIds.push_back(string("/="));
  else if(pet_expr_op_get_type(expr) == pet_op_and_assign)
    stmtOpIds.push_back(string("&="));
  else if(pet_expr_op_get_type(expr) == pet_op_xor_assign)
    stmtOpIds.push_back(string("^="));
  else if(pet_expr_op_get_type(expr) == pet_op_or_assign)
    stmtOpIds.push_back(string("|="));

  else if(pet_expr_op_get_type(expr) == pet_op_assign)
    stmtOpIds.push_back(string("="));
  else if(pet_expr_op_get_type(expr) == pet_op_add)
    stmtOpIds.push_back(string("+"));
  else if(pet_expr_op_get_type(expr) == pet_op_sub)
    stmtOpIds.push_back(string("-"));
  else if(pet_expr_op_get_type(expr) == pet_op_mul)
    stmtOpIds.push_back(string("*"));
  else if(pet_expr_op_get_type(expr) == pet_op_div)
    stmtOpIds.push_back(string("/"));
  else if(pet_expr_op_get_type(expr) == pet_op_mod)
    stmtOpIds.push_back(string("%"));
  else if(pet_expr_op_get_type(expr) == pet_op_eq)
    stmtOpIds.push_back(string("=="));
  else if(pet_expr_op_get_type(expr) == pet_op_ne)
    stmtOpIds.push_back(string("!="));
  else if(pet_expr_op_get_type(expr) == pet_op_address_of)
    stmtOpIds.push_back(string("&"));

  else if(pet_expr_op_get_type(expr) == pet_op_and 
      or pet_expr_op_get_type(expr) == pet_op_land)
    stmtOpIds.push_back(string("&"));
  else if(pet_expr_op_get_type(expr) == pet_op_xor)
    stmtOpIds.push_back(string("&"));
  else if(pet_expr_op_get_type(expr) == pet_op_or
      or pet_expr_op_get_type(expr) == pet_op_lor)
    stmtOpIds.push_back(string("&"));
  else if(pet_expr_op_get_type(expr) == pet_op_not
      or pet_expr_op_get_type(expr) == pet_op_lnot)
    stmtOpIds.push_back(string("&"));

  else if(pet_expr_op_get_type(expr) == pet_op_shl)
    stmtOpIds.push_back(string("<<"));
  else if(pet_expr_op_get_type(expr) == pet_op_shr)
    stmtOpIds.push_back(string(">>"));
  else if(pet_expr_op_get_type(expr) == pet_op_le)
    stmtOpIds.push_back(string("<="));
  else if(pet_expr_op_get_type(expr) == pet_op_ge)
    stmtOpIds.push_back(string(">="));

  else if(pet_expr_op_get_type(expr) == pet_op_lt)
    stmtOpIds.push_back(string("<"));
  else if(pet_expr_op_get_type(expr) == pet_op_gt)
    stmtOpIds.push_back(string(">"));

  else if(pet_expr_op_get_type(expr) == pet_op_minus)
    stmtOpIds.push_back(string("-"));
  else if(pet_expr_op_get_type(expr) == pet_op_post_inc)
    stmtOpIds.push_back(string("++"));
  else if(pet_expr_op_get_type(expr) == pet_op_post_dec)
    stmtOpIds.push_back(string("--"));
  else if(pet_expr_op_get_type(expr) == pet_op_pre_inc)
    stmtOpIds.push_back(string("++"));
  else if(pet_expr_op_get_type(expr) == pet_op_pre_dec)
    stmtOpIds.push_back(string("--"));
  else 
    cout << "Operation Id not necessary" << endl;
}


void GatherStmtVarIds(vector<string>& stmtVarIds, struct pet_expr* expr, isl_set* domainSet){
  if (pet_expr_get_type(expr) == pet_expr_op){
    if (pet_expr_get_type(expr) == pet_op_assume
        || pet_expr_get_type(expr) == pet_op_kill 
        || pet_expr_get_type(expr) == pet_op_assume 
        || pet_expr_get_type(expr) == pet_op_cond
        || pet_expr_get_type(expr) == pet_op_last
        ) return;
    GatherStmtOpIds(stmtVarIds, expr);
  }

  if (pet_expr_get_type(expr) == pet_expr_access){
    if(pet_expr_access_is_write(expr)){
      isl_map* may_write = isl_map_from_union_map(pet_expr_access_get_may_write(expr));
      isl_map* wmap = isl_map_intersect_domain(may_write,isl_set_copy(domainSet));
      
      const char* wchar = isl_map_get_tuple_name(wmap, isl_dim_out);
      if (wchar) stmtVarIds.push_back(string(wchar));
    }

    if(pet_expr_access_is_read(expr)){
      isl_map* may_read = isl_map_from_union_map(pet_expr_access_get_may_read(expr));
      isl_map* rmap = isl_map_intersect_domain(may_read,isl_set_copy(domainSet));

      const char* rchar = isl_map_get_tuple_name(rmap, isl_dim_out);
      if(rchar) stmtVarIds.push_back(rchar);
    }
  }else if (pet_expr_get_type(expr) == pet_expr_int){
    string int_str = to_string(isl_val_sgn(pet_expr_int_get_val(expr))); 
    //cout << "integer: " << int_str << endl;
    stmtVarIds.push_back(int_str);
  }else if (pet_expr_get_type(expr) == pet_expr_double){
    const char* double_char = pet_expr_double_get_str(expr); 
    //cout << "double:" << double_char << endl;
    if(double_char) stmtVarIds.push_back(double_char);
  }

  for(int i=0; i<pet_expr_get_n_arg(expr); i++) {
    GatherStmtVarIds(stmtVarIds, pet_expr_get_arg(expr,i), domainSet);
  }
}

std::vector<std::string> iter_names(isl_set* iset) {
    std::vector<std::string> ret;
    for(size_t j = 0; j < isl_set_n_dim(iset); j++) {
        const char* iname = isl_set_get_dim_name(iset, isl_dim_set, j);
        if(iname != nullptr)
            ret.push_back(std::string(iname));
        else
            ret.push_back(std::string("c" + std::to_string(j)));
    }
    return ret;
}

std::vector<std::string> iter_names(isl_space* space) {
    std::vector<std::string> ret;
    std::cerr<<"SPACE ndim="<<isl_space_dim(space, isl_dim_set)<<"\n";
    for(size_t j = 0; j < isl_space_dim(space, isl_dim_set); j++) {
        const char* iname = isl_space_get_dim_name(space, isl_dim_set, j);
        if(iname != nullptr)
            ret.push_back(std::string(iname));
        else
            ret.push_back(std::string("c" + std::to_string(j)));
    }
    return ret;
}

template<typename T>
std::string join(const std::vector<T>& vec, const std::string& sep) {
    using std::to_string;
    using islw::to_string;
    std::string ret;
    if(!vec.empty()) {
        ret += to_string(vec.front());
    }
    for(size_t i=1; i<vec.size(); i++) {
        ret += sep + to_string(vec[i]);
    }
    return ret;
}

template<>
std::string join(const std::vector<std::string>& vec, const std::string& sep) {
    using std::to_string;
    using islw::to_string;
    std::string ret;
    if(!vec.empty()) {
        ret += vec.front();
    }
    for(size_t i=1; i<vec.size(); i++) {
        ret += sep + vec[i];
    }
    return ret;
}


class Statement {
    public:
    Statement() : stmt_id_{-1}, domain_{nullptr}, write_ref_{nullptr} {}

    Statement(const Statement& stmt) : stmt_id_{stmt.stmt_id_} {
        domain_    = islw::copy(stmt.domain_);
        write_ref_ = islw::copy(stmt.write_ref_);
        for(const auto& rr : stmt.read_refs_) {
            read_refs_.push_back(islw::copy(rr));
        }
        for(const auto& rrc : stmt.read_ref_cards_) {
            read_ref_cards_.push_back(islw::copy(rrc));
        }
        // read_ref_macro_names_ = stmt.read_ref_macro_names_;
        read_ref_macro_args_ = stmt.read_ref_macro_args_;
        read_ref_macro_exprs_ = stmt.read_ref_macro_exprs_;
        array_sizes_ = stmt.array_sizes_;
        read_array_names_ = stmt.read_array_names_;
        write_array_name_ = stmt.write_array_name_;
        write_array_size_ = stmt.write_array_size_;
        stmt_varids_ = stmt.stmt_varids_;
    }

    Statement& operator = (Statement&& stmt){
        std::swap(*this, stmt);
        return *this;
    }

    Statement(int stmt_id, pet_scop* pscop) : stmt_id_{stmt_id} {
        isl_union_map* R     = pet_scop_get_may_reads(pscop);
        isl_union_map* W     = pet_scop_get_may_writes(pscop);
        isl_schedule* isched = pet_scop_get_schedule(pscop);
        isl_union_map* S     = isl_schedule_get_map(isched);

        domain_ = islw::copy(pscop->stmts[stmt_id_]->domain);

        // std::cout << "DOMAIN=\n"<<islw::to_string(domain_) << "\n";

        // std::cerr<<__FUNCTION__<<"\n R-----\n"<<islw::to_string(R)<<"\n-----\n";
        // std::cerr<<__FUNCTION__<<"\n W-----\n"<<islw::to_string(W)<<"\n-----\n";
        // std::cerr<<__FUNCTION__<<"\n S-----\n"<<islw::to_string(S)<<"\n-----\n";
        // std::cerr<<__FUNCTION__<<"\n isched-----\n"<<islw::to_string(isched)<<"\n-----\n";
        gather_references(pet_tree_expr_get_expr(pscop->stmts[stmt_id]->body));

        GatherStmtVarIds(stmt_varids_, pet_tree_expr_get_expr(pscop->stmts[stmt_id]->body), domain_);

#if 0
        isl_union_set* T = isl_union_map_range(islw::copy(S));
        isl_union_map* LT =
          isl_union_map_from_map(isl_map_lex_lt(isl_union_set_get_space(T)));
        isl_union_map* TW =
          islw::umap_compose_left(isl_union_map_reverse(islw::copy(S)), W);

        pet_stmt* pstmt = pscop->stmts[stmt_id];
        domain_ = isl_set_coalesce(
          isl_set_remove_redundancies(islw::copy(pstmt->domain)));

        // sched_ =
        //   isl_union_map_intersect_domain(islw::copy(S), islw::copy(domain_));

#endif
        std::cout<<"#read refs="<<read_refs_.size()<<"\n";
        if(write_ref_) {
            std::cout<<"WRITE REF FOUND\n";
        }
        gather_card(S, R,W);
        assert(read_refs_.size() == read_ref_cards_.size());
        construct_read_ref_macros();
        islw::destruct(R, W, isched, S);
    }

    ~Statement() {
        islw::destruct(write_ref_);
        islw::destruct(domain_);
        for(auto rr: read_refs_) {
            islw::destruct(rr);
        }
        for(auto rrc: read_ref_cards_) {
            islw::destruct(rrc);
        }
    }

    std::string read_ref_macro_defs() const {
        std::string ret;
        // assert(read_refs_.size() == read_ref_macro_names_.size());
        assert(read_ref_macro_names_.size() == read_ref_macro_args_.size());
        assert(read_ref_macro_names_.size() == read_ref_macro_exprs_.size());
        std::cerr<<"NUM DEF MACROS="<<read_refs_.size()<<"\n";
        for(size_t i = 0; i < read_refs_.size(); i++) {
            std::cerr<<"name[i]="<<read_ref_macro_name(i)<<"\n";
            std::cerr<<"args[i]="<<read_ref_macro_args_[i]<<"\n";
            std::cerr<<"exprs[i]="<<read_ref_macro_exprs_[i]<<"\n";
            ret += "#define " + read_ref_macro_name(i) +
                   read_ref_macro_args_[i] + "\t("+
                   read_ref_macro_exprs_[i] + ")\n";
        std::cerr<<"DEFS=\n"<<ret<<"\n";
        }
        std::cerr<<"DEFS=\n"<<ret<<"\n";
        return ret;
    }

    std::string read_ref_macro_undefs() const {
        std::string ret;
        // assert(read_refs_.size() == read_ref_macro_names_.size());
        for(size_t i = 0; i < read_refs_.size(); i++) {
            ret += "#undef " + read_ref_macro_name(i) + "\n";
        }
        std::cerr<<"UNDEFS=\n"<<ret<<"\n";
        return ret;
    }

    std::string inline_checks() const {
        std::string str   = "{\n";
        std::string sname = statement_name();
        assert(read_refs_.size() == array_sizes_.size());
        assert(read_refs_.size() == read_array_names_.size());
        if(write_ref_) {
            for(auto j = 0; j < write_array_size_; j++) {
                str += "int " + write_ref_dim_string(j)+
                       " = " + "/*to be filled by ajay*/" +
                       ";\n";
            }
        }
        for(size_t i = 0; i < read_refs_.size(); i++) {
            for(auto j=0; j<array_sizes_[i]; j++) {
                str += "int " +  read_ref_dim_string(i, j) +
                //  sname + "__" + std::to_string(i) + "__" +
                //        std::to_string(j) + 
                       " = " + "/*to be filled by ajay*/"+";\n";
            }
        }
        str += "\n";
        for(auto i = 0U; i < read_refs_.size(); i++) {
            str += "_diff |= " + read_ref_macro_name(i) + "(...)" +
            //  sname + "__" + std::to_string(i) + "(...)" +
                   "^" + "(int)(" + read_ref_string(i) + ");\n";
            //         read_array_names_[i];
            // if(array_sizes_[i] != 0) {
            //     str += "[" + sname + "__" + std::to_string(i) + "__" +
            //            std::to_string(0);
            // }
            // for(auto j = 1; j < array_sizes_[i]; j++) {
            //     str += "][" + sname + "__" + std::to_string(i) + "__" +
            //            std::to_string(j);
            // }
            // if(array_sizes_[i] != 0) { str += "]"; }
            // str += +");\n";
        }
        str += "\n";
        if(write_ref_ != nullptr) {
            // str += write_array_name_;
            // if(write_array_size_ != 0) {
            //     str += "[" + sname + "__w__" + std::to_string(0);
            // }
            // for(auto j = 1; j < write_array_size_; j++) {
            //     str += "][" + sname + "__w__" + std::to_string(j);
            // }
            // if(write_array_size_ != 0) { str += "]"; }
            str += write_ref_string();
            str += " += 1;\n";
        }
        str += "\n}\n";
        return str;
    }

    std::vector<string> stmt_varids() const {
      return stmt_varids_;
    }

    private:
    std::string statement_name() const {
        return std::string{"_s"} + std::to_string(stmt_id_);
    }

    std::string read_ref_dim_string(int read_ref_id, int dim_id) const {
        return statement_name() + "_r" + std::to_string(read_ref_id) + "_" +
               std::to_string(dim_id);
    }

    std::string read_ref_string(int read_ref_id) const {
      assert(read_ref_id >=0);
      assert(read_ref_id < read_array_names_.size());
      assert(read_ref_id < array_sizes_.size());
      std::vector<std::string> dim_strings;
      for(int i=0; i<array_sizes_[read_ref_id]; i++) {
        dim_strings.push_back(read_ref_dim_string(read_ref_id, i));
      } 
      if(dim_strings.size() > 0) { 
        return read_array_names_[read_ref_id] + "[" + join(dim_strings, "][") + "]";
      } else {
        return read_array_names_[read_ref_id];
      }
    }

    std::string write_ref_dim_string(int dim_id) const {
        return statement_name() + "_w_" + std::to_string(dim_id);
    }

    std::string write_ref_string() const {
      assert(write_ref_ != nullptr);
      std::vector<std::string> dim_strings;
      for(int i=0; i<write_array_size_; i++) {
        dim_strings.push_back(write_ref_dim_string(i));
      } 
      if(dim_strings.size() > 0) {
        return write_array_name_ + "[" + join(dim_strings, "][") + "]";
      } else {
        return write_array_name_;
      }
    }

    void gather_references(pet_expr* expr) {
        if(pet_expr_get_type(expr) == pet_expr_access) {
            if(pet_expr_access_is_read(expr)) {
                isl_map* may_read =
                  isl_map_from_union_map(pet_expr_access_get_may_read(expr));
                // std::cerr<<"MAY READ=\n"<<islw::to_string(may_read)<<"\n";
                read_refs_.push_back(isl_map_intersect_domain(
                  may_read, islw::copy(domain_)));
                isl_id* id = pet_expr_access_get_id(expr);
                read_array_names_.push_back(islw::to_string(id));
                islw::destruct(id);
                  #if 1
                // std::cout << "&&&narg"
                //           << islw::to_string(isl_set_from_union_set(
                //                isl_union_map_range(
                //                  pet_expr_access_get_may_read(expr))))
                //           << "\n";
                // std::cout << "&&&narg"
                //           << isl_set_dim(isl_set_from_union_set(
                //                isl_union_map_range(
                //                  pet_expr_access_get_may_read(expr))),
                //                isl_dim_set)
                //           << "\n";
                // std::cout<<"ID----:"<<islw::to_string(pet_expr_access_get_ref_id(expr))<<"\n";
                // std::cout<<"&&&&&expr narg="<<pet_expr_get_n_arg(expr)<<"\n";
                //                           << isl_set_dim(isl_set_from_union_set(
                array_sizes_.push_back(isl_set_dim(isl_set_from_union_set(
                  isl_union_map_range(pet_expr_access_get_may_read(expr))),
                  isl_dim_set));
                std::cout<<"&&&& DIM "<<read_array_names_.back()<<" = "<<array_sizes_.back()<<"\n";
                // array_sizes_.push_back(pet_expr_get_n_arg(expr));
                #else
                array_sizes_.push_back(1);
                #endif
                // std::cerr<<"READS=\n"<<islw::to_string(read_refs_.back())<<"\n";
                // read_ref_cards_.push_back(
                //   isl_union_map_card(islw::copy(read_refs_.back())));
            }

            if(pet_expr_access_is_write(expr)) {
                isl_map* may_write =
                  isl_map_from_union_map(pet_expr_access_get_may_write(expr));
                write_ref_ = isl_map_intersect_domain(
                  may_write, islw::copy(domain_));
                isl_id* id = pet_expr_access_get_id(expr);
                write_array_name_ = islw::to_string(id);
                islw::destruct(id);
                write_array_size_ = isl_set_dim(isl_set_from_union_set(
                  isl_union_map_range(pet_expr_access_get_may_write(expr))),
                  isl_dim_set);

                // const char* writeArray =
                //   isl_map_get_tuple_name(write_ref_, isl_dim_out);
                // statement->setWriteArrayName(string(writeArray));
            }
        }
        for(int i = 0; i < pet_expr_get_n_arg(expr); i++) {
            gather_references(pet_expr_get_arg(expr, i));
        }
        pet_expr_free(expr);
    }

    /**
     * @brief 
     * 
     * read_cards_[i] = card S . (((S^-1) . read_refs_[i] .(TW^-1)) * LT )
     * 
     */
    void gather_card(isl_union_map* S, isl_union_map* R, isl_union_map* W) {
        //LT = T << T
        //Sinv = S^-1
        //TWinv = (Sinv . W)^-1
        isl_union_map* Sinv = isl_union_map_reverse(islw::copy(S));
        isl_union_map* sinstances = isl_union_map_union(islw::copy(R), islw::copy(W));
        isl_union_set* T = isl_union_map_range(isl_union_map_intersect_domain(
          islw::copy(S), isl_union_map_domain(islw::copy(sinstances))));
        isl_union_map* LT =
        //   isl_union_map_from_map(isl_map_lex_lt(isl_union_set_get_space(T)));
        isl_union_set_lex_lt_union_set(islw::copy(T), islw::copy(T));
        isl_union_map* TWinv =
          isl_union_map_reverse(islw::umap_compose(Sinv, W));

        for(size_t i=0; i<read_refs_.size(); i++) {
            isl_union_map* T_to_prevW = isl_union_map_intersect(
              islw::copy(LT),
              islw::umap_compose(
                Sinv, isl_union_map_from_map(islw::copy(read_refs_[i])),
                TWinv));
            isl_union_map* S_to_prevW = islw::umap_compose(S, T_to_prevW);
            read_ref_cards_.push_back(isl_union_map_card(islw::copy(S_to_prevW)));
            std::cerr << "STMT " << stmt_id_ << " READ " << i<<"\n";
            std::cerr<<"read_refs:\n"<<islw::to_string(read_refs_[i])<<"\n";
            std::cerr<<"LT:\n"<<islw::to_string(LT)<<"\n";
            std::cerr<<"Space(T): "<<islw::to_string(isl_union_set_get_space(T))<<"\n";
            std::cerr<<"T:\n"<<islw::to_string(T)<<"\n";
            std::cerr<<"TWinv:\n"<<islw::to_string(TWinv)<<"\n";
            std::cerr<<"Sinv:\n"<<islw::to_string(Sinv)<<"\n";
            std::cerr<<"T_to_prevW:\n"<<islw::to_string(T_to_prevW)<<"\n";
            std::cerr<<"S_to_prevW:\n"<<islw::to_string(S_to_prevW)<<"\n";

            std::cerr              << " CARD:\n-----\n";

            std::cout<<islw::to_string(read_ref_cards_.back())<<"\n------\n";

            std::cout << "pw\n-------\n"
                      << isl_pw_qpolynomial_to_str(isl_union_pw_qpolynomial_extract_pw_qpolynomial(
                          read_ref_cards_.back(),
                           isl_union_pw_qpolynomial_get_space(
                             read_ref_cards_.back())))
                      << "\n----\n";
            // std::cout<<islw::to_c_string(isl_union_pw_qpolynomial_to_polynomial()
            // read_ref_cards_.back())<<"\n------\n";
            islw::destruct(T_to_prevW, S_to_prevW);
        }
        islw::destruct(Sinv, sinstances, T, LT, TWinv);
    }

    void construct_read_ref_macros() {
        for(size_t i=0; i<read_refs_.size(); i++) {
            std::string str;
            // std::cerr<<"STMT "<<stmt_id_<<" READ "<<i<<" SPACE:\n"
            // <<islw::to_string(isl_map_get_space(read_refs_[i]))<<"\n---\n";
            // // std::cerr<<"STMT "<<stmt_id_<<" READ "<<i<<" SPACE NAME:\n"
            // // <<islw::to_string(isl_space_get_name(isl_map_get_space(read_refs_[i])))<<"\n---\n";
            // std::cerr<<"STMT "<<stmt_id_<<" READ "<<i<<" SPACE TUPLE NAME:\n"
            // <<isl_space_get_tuple_name(isl_map_get_space(read_refs_[i]), isl_dim_in)<<"\n---\n";

            // str = std::string{isl_space_get_tuple_name(isl_map_get_space(read_refs_[i]), isl_dim_in)}+
            // "__"+ std::to_string(i);
            // read_ref_macro_names_.push_back(str);

            // std::cerr<<"DOMAIN="<<islw::to_string(isl_space_domain(islw::copy(isl_map_get_space(read_refs_[i]))))<<"\n";

            std::string iname = join(iter_names(isl_space_domain(islw::copy(
                                       isl_map_get_space(read_refs_[i])))),
                                     ",");
            read_ref_macro_args_.push_back("(" + iname + ")");
            std::cerr << "RR MACRO= #define " << read_ref_macro_name(i)
                      << "     "<< read_ref_macro_args_.back() 
                      <<"    "<<islw::to_string(read_ref_cards_[i])<<"\n";
        }
        construct_read_ref_macro_exprs();
    }

    std::string read_ref_macro_name(int read_ref_id) const {
        return std::string{"V_S"} + std::to_string(stmt_id_) + "_r" +
               std::to_string(read_ref_id);
    }
    void construct_read_ref_macro_exprs() {
        for(size_t i=0; i<read_ref_cards_.size(); i++) {
            // read_ref_macro_exprs_.push_back(islw::to_string(read_ref_cards_[i]));
            std::string str;
            isl_union_pw_qpolynomial_foreach_pw_qpolynomial(
              read_ref_cards_[i], print_pw_qpoly, &str);
            if(str.empty()) { str = std::to_string(0); }
            read_ref_macro_exprs_.push_back(str);
        }        
    }

    int stmt_id_;
    isl_set* domain_;
    std::vector<isl_map*> read_refs_;
    std::vector<std::string> read_array_names_;
    std::vector<int> array_sizes_; //dimensionality of the i-th read array reference 
    std::vector<isl_union_pw_qpolynomial*> read_ref_cards_;
    // std::vector<std::string> read_ref_macro_names_;
    std::vector<std::string> read_ref_macro_args_;
    std::vector<std::string> read_ref_macro_exprs_;
    //std::vector<std::string> inline_checks_;
    isl_map* write_ref_;
    std::string write_array_name_;
    int write_array_size_;
    std::vector<string> stmt_varids_;
    // isl_union_map* sched_;
};

struct PdLoc {
        PdLoc() : end(0) {}

        unsigned start;
        unsigned end;
}; // struct PdLoc {

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
  MyASTVisitor (Preprocessor &PP, Rewriter  &R, PdLoc &loc, std::vector<Statement> stmts, string prologue, string epilogue)
    :  TheRewriter(R), loc(loc), PP(PP), FaultTolerantRegionStarted(false), stmts(stmts), Prologue(prologue), Epilogue(epilogue), m_inForLine(false)
  {}
  
  bool TraverseForStmt(ForStmt *fs)// Traverse for stmt to make mark
  {
    SourceManager &SM = PP.getSourceManager();

    /*Standard checking begins*/
    if (SM.getFileOffset(fs->getLocStart()) <= loc.start)
      return true;
    if (SM.getFileOffset(fs->getLocEnd()) >= loc.end)
    {
      if (FaultTolerantRegionStarted == true)
      {
        FaultTolerantRegionStarted = false;
      }// if (FaultTolerantRegionStarted == true)
      return true;
    }
    /*Standard checking ends*/

    // Enter scop region
    if (!WalkUpFromForStmt(fs))
      return false;

    m_inForLine = true;

      clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

    for (Stmt::child_iterator range = fs->child_begin(); range!=fs->child_end(); ++range)
    {
      if(*range == fs->getBody()){
        m_inForLine = false;
        TraverseStmt(fs->getBody());
      }

      // cout << "------------------body start-------" << m_inForLine << "----------------\n";
      // string fbdy;
      // llvm::raw_string_ostream fbdyst(fbdy);
      //  fs->getBody()->printPretty(fbdyst,NULL,Policy);
      //  cout << fbdyst.str() << endl;
      // cout << "------------------body end----------------\n";
      
    }
    return true;
  }//bool TraverseForStmt(ForStmt *fs)


  bool TraverseExprToCheckArray (Expr *expr)
  {
    if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    {
	    BinaryOperator* b = cast<BinaryOperator>(expr);
	    return ( TraverseExprToCheckArray (b->getLHS()) || TraverseExprToCheckArray (b->getRHS()) );
    }// if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    else if (expr->getStmtClass() == Stmt::IntegerLiteralClass || expr->getStmtClass() == Stmt::FloatingLiteralClass)
    {
	    // do nothing
      return false;
    }// else if (expr->getStmtClass() == Stmt::IntegerLiteralClass)
    else if (expr->getStmtClass() == Stmt::ParenExprClass)
    {
	    ParenExpr *p = cast<ParenExpr>(expr);
	    return TraverseExprToCheckArray(p->getSubExpr());
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ImplicitCastExprClass)
    {
	    ImplicitCastExpr *i = cast<ImplicitCastExpr>(expr);
	    return TraverseExprToCheckArray(i->getSubExpr());
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ArraySubscriptExprClass)// isa<ArraySubscriptExpr>(expr) 
    {
      return true;
    }//else if (expr->getStmtClass() == Stmt::ArraySubscriptExprClass)
    else
    {
      return false;
    }//else
  }// bool TraverseExprToCheckArray (Expr *expr)
 

  void TraverseExprToGetStmtIters (Expr *expr, vector<string> &vecIters, vector<string> &varIds)
  {
    if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    {
	    BinaryOperator* b = cast<BinaryOperator>(expr);
      varIds.push_back(string((b->getOpcodeStr(b->getOpcode())).str()));//operator
	    TraverseExprToGetStmtIters (b->getLHS(), vecIters, varIds);
	    TraverseExprToGetStmtIters (b->getRHS(), vecIters, varIds);
    }// if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    else if (expr->getStmtClass() == Stmt::IntegerLiteralClass || expr->getStmtClass() == Stmt::FloatingLiteralClass)
    {
	    // do nothing
    }// else if (expr->getStmtClass() == Stmt::IntegerLiteralClass)
    else if (expr->getStmtClass() == Stmt::ParenExprClass)
    {
	    ParenExpr *p = cast<ParenExpr>(expr);
	    TraverseExprToGetStmtIters(p->getSubExpr(), vecIters, varIds);
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ImplicitCastExprClass)
    {
	    ImplicitCastExpr *i = cast<ImplicitCastExpr>(expr);
	    TraverseExprToGetStmtIters(i->getSubExpr(), vecIters, varIds);
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ArraySubscriptExprClass)
    {
      ArraySubscriptExpr* arrayExpr = cast<ArraySubscriptExpr>(expr);
      TraverseExprToGetStmtIters (arrayExpr->getLHS(), vecIters, varIds);
			//TraverseExprToGetStmtIters (arrayExpr->getRHS(), vecIters);
      
      // store vector iterators 
      clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

	    string vecIterStr;
      llvm::raw_string_ostream vecIter(vecIterStr);
	    arrayExpr->getRHS()->printPretty(vecIter, NULL, Policy);
			//cout << "Idx: " << s.str() << endl;
	    vecIters.push_back(vecIter.str());

    }// isa<ArraySubscriptExpr>(expr) 
    else// DeclRefExpr: vector name, variable name
    {
      // store vector and variable names
      clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

      string varIdStr;
      llvm::raw_string_ostream varId(varIdStr);
      expr->printPretty(varId, NULL, Policy);
      //cout << "var ids: " << varId.str() << endl;
      varIds.push_back(varId.str());

	  }// else
  }//void TraverseExprToGetStmtIters 

  bool CheckStmtVarIds(vector<string> stmtVarIds, vector<string> petStmtVarIds)
  {
    if (stmtVarIds.size() != petStmtVarIds.size())
      return false;
      
    for (auto i=0U; i<stmtVarIds.size(); i++)
    {
      if ((stmtVarIds[i]).compare(petStmtVarIds[i]) != 0)
        return false;
    }
    return true;
    
  }
 
  bool VisitBinaryOperator(BinaryOperator *b)
  {
    /*Standard checking begins*/
    SourceManager &SM = PP.getSourceManager();
    if (SM.getFileOffset(b->getLocStart()) <= loc.start)
      return true;
    if (SM.getFileOffset(b->getLocEnd()) >= loc.end)
    {
	    if (FaultTolerantRegionStarted == true)
	    {
	      FaultTolerantRegionStarted = false;
	      TheRewriter.InsertTextBefore(b->getLocStart(), "// Fault tolerance ends here\n");
	    }// if (FaultTolerantRegionStarted == true)
      return true;
    }
    /*Standard checking ends*/
  #if 1
    // Entering the fault tolerant region
    if (b->isAssignmentOp() || b->isCompoundAssignmentOp())
    {
      if (!m_inForLine)// check BinaryOperator not in for loop
      {
        Expr *lhs = b->getLHS();
        Expr *rhs = b->getRHS();

        bool lhsArray = isa<ArraySubscriptExpr>(lhs);
        bool rhsArray = TraverseExprToCheckArray(rhs);

        // Check if array reference Statement
        // if true perform insertion, else do nothing
        if ( lhsArray || rhsArray )
        {
          // comment out the orginal stmt
          SourceLocation commentLocation = b->getLocStart();
	        // TheRewriter.InsertText(commentLocation, "{\n//", true, true);

          vector<string> stmtVecIters;        // collect stmt index
          vector<string> stmtVarIds;          // collect stmt var ids
          stmtVarIds.push_back(string((b->getOpcodeStr(b->getOpcode())).str()));//operator
          TraverseExprToGetStmtIters (lhs, stmtVecIters, stmtVarIds);

          // Fix bug for CompoundAssignmentOp
          if (b->isCompoundAssignmentOp())
          {
            TheRewriter.InsertText(commentLocation, "//", true, true);

            // add stmt var ids for compound assignment operator
            if (stmtVarIds.size() > 0 )
            {
              int len = stmtVarIds.size();
              for (int k=1; k<len; k++)
              //for (int ca=0; ca<stmtVarIds.size(); ca++)
              {
                //cout << "stmtVarIds: " << stmtVarIds[k] << endl;
                stmtVarIds.push_back(stmtVarIds[k]);
              }// end for
            }// end stmtVarIds

            // add stmt vector iters for compound assignment operator
            if (stmtVecIters.size() > 0)
            {
              int len = stmtVecIters.size();
              for (int k=0; k<len; k++)
              {
                ////cout << "stmtVarIds: " << stmtVarIds[i] << endl;
                stmtVecIters.push_back(stmtVecIters[k]);
              }// end for
            }
            
          }//if (b->isCompoundAssignmentOp())
          // else do nothing

          TraverseExprToGetStmtIters (rhs, stmtVecIters, stmtVarIds);

          // Insert location for each statement
          SourceLocation END = b->getLocEnd();
	        int offset = Lexer::MeasureTokenLength(END,
	                     pTheCompInst_g->getSourceManager(),
	                     pTheCompInst_g->getLangOpts()) + 1;
	        SourceLocation END1 = END.getLocWithOffset(offset);

          TheRewriter.InsertText(END1, "\n//---begin checks---\n", true, true);
          
          // iterate all stmts to find the right one
          for (auto i=0U; i < stmts.size(); i++)
          {
            vector<string> petStmtVarIds = stmts[i].stmt_varids();
            bool equal = CheckStmtVarIds(stmtVarIds, petStmtVarIds);
            if (equal)
              TheRewriter.InsertText(END1, stmts[i].inline_checks(), true, true);
          } //

          TheRewriter.InsertText(END1, "//---end checks---\n", true, true);
          

        }//if ( lhsArray || rhsArray)
      }//if (!m_inForLine)
    }// if (b->isAssignmentOp())
    #endif

    return true;
  }//bool VisitBinaryOperator(BinaryOperator *b)

  bool VisitStmt(Stmt *s)
  {
      SourceManager &SM = PP.getSourceManager();

      if (SM.getFileOffset(s->getLocStart()) <= loc.start)
        return true;

      // else if(SM/getFileOffset(s->getLocStart())>loc.start)
      if (SM.getFileOffset(s->getLocEnd()) >= loc.end)
      {
	      if (FaultTolerantRegionStarted == true)
	      {
	        FaultTolerantRegionStarted = false;
	      }// if (FaultTolerantRegionStarted == true)
        return true;
      }

      // else if(SM.getFileOffset(s->getLocStart())>loc.start&&SM.getFileOffset(s->getLocEnd()) < loc.end)
      if (FaultTolerantRegionStarted == false)
      {
      	FaultTolerantRegionStarted = true;

        // ADD Prologue

            TheRewriter.InsertTextAfter(s->getLocStart(), "// Prologue\n");
            TheRewriter.InsertTextAfter(s->getLocStart(), Prologue);
            TheRewriter.InsertTextAfter(s->getLocStart(), "\n");
      }// if (FaultTolerantRegionStarted == false)

      //static int divid = 0;
      if (isa<IfStmt>(s))
      {
	      IfStmt *IfStatement = cast<IfStmt>(s);
	      Stmt *Then = IfStatement->getThen();

	      // PresumedLoc PLoc = pTheCompInst_g->getSourceManager().getPresumedLoc(IfStatement->getLocStart());//, 1);
	      // cout << "Line number is " << PLoc.getLine() << endl;
	      // cout << "Line number is " << pTheCompInst_g->getSourceManager().getSpellingLineNumber(IfStatement->getLocStart(), 0) << endl;
	      // cout << "Line number is " << pTheCompInst_g->getSourceManager().getExpansionLineNumber(IfStatement->getLocStart(), 0) << endl;
	      // cout << "Line At the end: number is " << pTheCompInst_g->getSourceManager().getPresumedLineNumber(IfStatement->getLocStart(), 0) << endl;
	      // cout << "-----------------------\n";

	      stringstream str_before;
	      //divid++;
	      TheRewriter.InsertText(Then->getLocStart(), str_before.str(), true, true);

	      SourceLocation END = Then->getLocEnd();
	      int offset = Lexer::MeasureTokenLength(END, pTheCompInst_g->getSourceManager(),	pTheCompInst_g->getLangOpts()) + 1;
	      SourceLocation END1 = END.getLocWithOffset(offset);

	      stringstream str_after;
	      TheRewriter.InsertText(END1, str_after.str(), true, true);

	      Stmt *Else = IfStatement->getElse();

	      if (Else)
	      {
	        // SourceLocation END = Else->getLocEnd();
	        // int offset = Lexer::MeasureTokenLength(END, pTheCompInst_g->getSourceManager(),
	      					//  pTheCompInst_g->getLangOpts()) + 1;
          // SourceLocation END1 = END.getLocWithOffset(offset);

	        TheRewriter.InsertText(Else->getLocStart(), "// the 'else' part\n", true, true);
	      }
      }

      return true;
  }//bool VisitStmt(Stmt *s)

  bool VisitFunctionDecl (FunctionDecl *f)
  {
    //  if (f->hasBody())
    //  {
	  //     Stmt *FunBody = f->getBody();
	  //     QualType QT = f->getCallResultType();
	  //     string TypeStr = QT.getAsString();

	  //     DeclarationName DeclName = f->getNameInfo().getName();
	  //     string FuncName = DeclName.getAsString();

	  //     stringstream SSBefore;
	  //     SSBefore << "// Epilogue Declaration Code in Global " << "\n";
    //     // for (int i=0; i<EpilogueDecl->size(); i++)
    //     //   SSBefore << *(EpilogueDecl->at(i)) ;

	  //     SSBefore << "// Begin function " << FuncName << " returning " << TypeStr << "\n";
	  //     SourceLocation ST = f->getSourceRange().getBegin();
	  //     TheRewriter.InsertText(ST, SSBefore.str()+Epilogue, true, true);

	  //     stringstream SSAfter;
	  //     SSAfter << "// End function " << FuncName << "\n";
	  //     ST = FunBody->getLocEnd().getLocWithOffset(1);
	  //     TheRewriter.InsertText(ST, SSAfter.str(), true, true);
    //  }

     return true;
  }

private:
  Rewriter &TheRewriter;
  PdLoc &loc;
  Preprocessor &PP;
  bool FaultTolerantRegionStarted;
  std::vector<Statement> stmts;
  string Prologue;
  string Epilogue;
  bool m_inForLine;
};

class MyASTConsumer : public ASTConsumer
{
public:
  MyASTConsumer(Preprocessor &PP, Rewriter &R, PdLoc &l, std::vector<Statement> stmts, string prologue, string epilogue) :
      Visitor (PP, R, l, stmts, prologue, epilogue), loc(l), PP(PP), stmts(stmts), Prologue(prologue), Epilogue(epilogue)
  {
  }

  virtual bool HandleTopLevelDecl (DeclGroupRef DR)
  {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
    {
      FunctionDecl *fd = dyn_cast<clang::FunctionDecl>(*b);
      if (!fd)
        continue;
      if (!fd->hasBody())
        continue;
      SourceManager &SM = PP.getSourceManager();

      if (SM.getFileOffset(fd->getLocStart()) > loc.end)
        continue;
      if (SM.getFileOffset(fd->getLocEnd()) < loc.start)
        continue;
      Visitor.TraverseDecl(*b);
    }
    return true;
  }// virtual bool HandleTopLevelDecl (DeclGroupRef DR)

private:
  MyASTVisitor Visitor;
  PdLoc &loc;
  Preprocessor &PP;
  std::vector<Statement> stmts;
  string Prologue;
  string Epilogue;

}; // class MyASTConsumer : public ASTConsumer

/* Handle pragmas of the form
 *      #pragma Pd
 * In particular, store the location of the line containing
 * the pragma in loc.start.
 */
struct PragmaPdHandler : public PragmaHandler {
  PdLoc &loc;
	Rewriter &TheRewriter;

  PragmaPdHandler(PdLoc &loc, Rewriter &R) : PragmaHandler("scop"), loc(loc), TheRewriter(R)
	{}


  virtual void HandlePragma(Preprocessor &PP,
                            PragmaIntroducerKind Introducer,
                            Token &ScopTok) {

      SourceManager &SM = PP.getSourceManager();
      SourceLocation sloc = ScopTok.getLocation();
      int line = SM.getExpansionLineNumber(sloc);
      sloc = SM.translateLineCol(SM.getFileID(sloc), line, 1);
      loc.start = SM.getFileOffset(sloc);

	    // TheRewriter.InsertTextAfter(sloc, "#include<assert.h>\n");
      // TheRewriter.InsertTextAfter(sloc,"#define ceild(n,d)  ceil(((double)(n))/((double)(d))) \n");
      // TheRewriter.InsertTextAfter(sloc,"#define floord(n,d) floor(((double)(n))/((double)(d))) \n");

      TheRewriter.InsertTextAfter(sloc, "#define max(x,y)    ((x) > (y)? (x) : (y))\n");
      TheRewriter.InsertTextAfter(sloc, "#define min(x,y)    ((x) < (y)? (x) : (y))\n");
	    // TheRewriter.InsertTextAfter(sloc, "int checksum;\n");
	    // TheRewriter.InsertTextAfter(sloc, "int version;\n");
      // The following insert is not necessary
      // since the pocc will keep the original 
      // iterator such as int i j k
			// TheRewriter.InsertTextAfter(sloc, "register int i, j, k;\n");

      //cout << "Pragma scop found at: " << loc.start << endl;
      }
};

/* Handle pragmas of the form
 *      #pragma Pd
 * In particular, store the location of the line containing
 * the pragma in loc.end.
 */

struct PragmaEndPdHandler : public PragmaHandler {
  PdLoc &loc;
	Rewriter &TheRewriter;
  string Epilogue;

  PragmaEndPdHandler(PdLoc &loc, Rewriter &R, string epilogue) : PragmaHandler("endscop"), loc(loc), TheRewriter(R), Epilogue(epilogue)
	{}

  virtual void HandlePragma(Preprocessor &PP,
                            PragmaIntroducerKind Introducer,
                            Token &ScopTok) {

      SourceManager &SM = PP.getSourceManager();
      SourceLocation sloc = ScopTok.getLocation();
      int line = SM.getExpansionLineNumber(sloc);
      sloc = SM.translateLineCol(SM.getFileID(sloc), line, 1);
      loc.end = SM.getFileOffset(sloc);
      //cout << "Pragma endscop found at: " << loc.end << endl;
      
      // ADD Epilogue
          TheRewriter.InsertTextBefore(sloc, Epilogue);
          TheRewriter.InsertTextBefore(sloc, "// Epilogue\n");
          TheRewriter.InsertTextBefore(sloc, "\n");
  }
};

void WriteToFile(string outputFileName, string output)
{
  ofstream ofs (outputFileName.c_str(), ofstream::out);
  ofs << output;
  ofs.close();
}// void WriteToFile(string outputFileName, string output)

// ParseScop: the main file 
int ParseScop(string fileName, std::vector<Statement> &stmts, string prologue, string epilogue, string outputFileName)
{

  // CompilerInstance will hold the instance of the Clang compiler,
  // managing the various objects needed to run the compiler.
  CompilerInstance TheCompInst;
  pTheCompInst_g = &TheCompInst;
  TheCompInst.createDiagnostics ();

  // Initialize target info with the default triple for our platform.
//  TargetOptions TO;
//  TO.Triple = llvm::sys::getDefaultTargetTriple();
  std::shared_ptr<clang::TargetOptions> TO = std::make_shared<clang::TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();

  TargetInfo *TI = TargetInfo::CreateTargetInfo(
		    TheCompInst.getDiagnostics(), TO);
  TheCompInst.setTarget(TI);

  TheCompInst.createFileManager();
  FileManager &FileMgr = TheCompInst.getFileManager();
  TheCompInst.createSourceManager (FileMgr);
  SourceManager &SourceMgr = TheCompInst.getSourceManager();
  TheCompInst.createPreprocessor(clang::TU_Complete);
  TheCompInst.createASTContext();

  // A Rewriter helps us manage the code rewriting task.
  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr.getFile(fileName);
  clang::FileID pcFileID = SourceMgr.createFileID(FileIn,clang::SourceLocation(), clang::SrcMgr::C_User);
  SourceMgr.setMainFileID(pcFileID);
  TheCompInst.getDiagnosticClient().BeginSourceFile(
       TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());


  Preprocessor &PP = TheCompInst.getPreprocessor();

  // Data structures to store #pragma PD, and #pragma endPD information
  PdLoc loc;
  PP.AddPragmaHandler(new PragmaPdHandler(loc, TheRewriter));
  PP.AddPragmaHandler(new PragmaEndPdHandler(loc, TheRewriter, epilogue));

  // Create an AST consumer instance which is going to get called by ParseAST
  //MyASTConsumer TheConsumer(PP, TheRewriter, loc, statements,returnValues.LiveDataCheckerCode);
  MyASTConsumer TheConsumer(PP,TheRewriter,loc,stmts, prologue,epilogue);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(PP, &TheConsumer, TheCompInst.getASTContext());

  // At this point the rewriter's buffer should be full with the rewritten file contents.
  const RewriteBuffer *RewriteBuf =
   TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());

  string Output = string (RewriteBuf->begin(), RewriteBuf->end());

  WriteToFile(outputFileName, Output);
  cout << "Wrote result to " << outputFileName << endl;
 // exit(-1);
  return 0;
}//int ParseScop(char *fileName, ReturnValues returnValues, string outputFileName)
