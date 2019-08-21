#pragma once

#include <vector>
#include <map>
#include <iostream>
#include <cassert>

#include "islw.hpp"
#include "util.hpp"
#include "array_pack.hpp"


static void GatherStmtVarIds(std::vector<std::string>& stmtVarIds, struct pet_expr* expr, isl_set* domainSet){
  if (pet_expr_get_type(expr) == pet_expr_op){
    if (pet_expr_op_get_type(expr) == pet_op_assume
        || pet_expr_op_get_type(expr) == pet_op_kill 
        || pet_expr_op_get_type(expr) == pet_op_assume 
        //|| pet_expr_op_get_type(expr) == pet_op_cond
        || pet_expr_op_get_type(expr) == pet_op_last
        ) {
          pet_expr_free(expr);
          return;
        }
    stmtVarIds.push_back(pet_op_str(pet_expr_op_get_type(expr)));
    // GatherStmtOpIds(stmtVarIds, expr);
  }

  if (pet_expr_get_type(expr) == pet_expr_access){
    if(pet_expr_access_is_write(expr)){
      isl_map* may_write = isl_map_from_union_map(pet_expr_access_get_may_write(expr));
      isl_map* wmap = isl_map_intersect_domain(may_write,isl_set_copy(domainSet));
      
      const char* wchar = isl_map_get_tuple_name(wmap, isl_dim_out);
      if (wchar) stmtVarIds.push_back(std::string(wchar));
      islw::destruct(wmap);
    }

    if(pet_expr_access_is_read(expr)){
      isl_map* may_read = isl_map_from_union_map(pet_expr_access_get_may_read(expr));
      isl_map* rmap = isl_map_intersect_domain(may_read,isl_set_copy(domainSet));

      const char* rchar = isl_map_get_tuple_name(rmap, isl_dim_out);
      if(rchar) stmtVarIds.push_back(rchar);
      islw::destruct(rmap);
    }
  }else if (pet_expr_get_type(expr) == pet_expr_int){
    isl_val* ival = pet_expr_int_get_val(expr);
    std::string int_str = std::to_string(stod(std::to_string(isl_val_sgn(ival))));
    islw::destruct(ival); 
    //cout << "integer: " << int_str << endl;
    stmtVarIds.push_back(int_str);
  }else if (pet_expr_get_type(expr) == pet_expr_double){
    const char* double_char = pet_expr_double_get_str(expr); 
    // cout << "pet double:" << double_char  << endl;
    if(double_char) stmtVarIds.push_back(std::to_string(std::stod(double_char))); 
  }

  for(int i=0; i<pet_expr_get_n_arg(expr); i++) {
    GatherStmtVarIds(stmtVarIds, pet_expr_get_arg(expr,i), domainSet);
  }
  pet_expr_free(expr);
}

static isl_stat print_pw_qpoly(__isl_take isl_pw_qpolynomial *pwqp, void *user) {
    assert(pwqp);
    assert(user);
    std::string& str = *(std::string*)user;
    // std::cout<<"pwqp in c:---\n"<<islw::to_c_string(pwqp)<<"\n---\n";
    str += islw::to_c_string(pwqp);
    islw::destruct(pwqp);
    return isl_stat_ok;
}

class Statement {
    public:
    Statement() :
      stmt_id_{-1},
      domain_{nullptr},
      write_ref_{nullptr},
      write_ref_card_{nullptr} {}

    Statement(const Statement& stmt) : stmt_id_{stmt.stmt_id_} {
        domain_    = islw::copy(stmt.domain_);

        R_ = islw::copy(stmt.R_);
        W_ = islw::copy(stmt.W_);
        isched_ = islw::copy(stmt.isched_);
        S_ = islw::copy(stmt.S_);
        read_array_names_ = stmt.read_array_names_;
        array_sizes_ = stmt.array_sizes_;
        for(const auto& rr : stmt.read_refs_) {
            read_refs_.push_back(islw::copy(rr));
        }
        for(const auto& rrc : stmt.read_ref_cards_) {
            read_ref_cards_.push_back(islw::copy(rrc));
        }
        // read_ref_macro_names_ = stmt.read_ref_macro_names_;
        read_ref_macro_args_ = stmt.read_ref_macro_args_;
        read_ref_macro_exprs_ = stmt.read_ref_macro_exprs_;
        for(auto &rdms: stmt.read_dim_maps_) {
          read_dim_maps_.push_back({});
          for(auto& rdm: rdms) {
            read_dim_maps_.back().push_back(islw::copy(rdm));
          }
        }
        read_dim_macro_args_ = stmt.read_dim_macro_args_;

        write_array_name_ = stmt.write_array_name_;
        write_array_size_ = stmt.write_array_size_;
        write_ref_ = islw::copy(stmt.write_ref_);
        write_ref_card_ = islw::copy(stmt.write_ref_card_);
        write_ref_macro_args_ = stmt.write_ref_macro_args_;
        write_ref_macro_exprs_ = stmt.write_ref_macro_exprs_;
        for(auto& wdm : stmt.write_dim_maps_) {
            write_dim_maps_.push_back(islw::copy(wdm));
        }
        write_dim_macro_args_ = stmt.write_dim_macro_args_;

        stmt_varids_ = stmt.stmt_varids_;
        sinstance_macro_exprs_ = stmt.sinstance_macro_exprs_;
        sinstance_macro_params_ = stmt.sinstance_macro_params_;
        sinstance_macro_args_ = stmt.sinstance_macro_args_;
    }

    Statement& operator = (Statement&& stmt){
        std::swap(*this, stmt);
        return *this;
    }

    Statement(int stmt_id, pet_scop* pscop) : stmt_id_{stmt_id} {
        R_      = pet_scop_get_may_reads(pscop);
        W_      = pet_scop_get_may_writes(pscop);
        isched_ = pet_scop_get_schedule(pscop);
        S_      = isl_schedule_get_map(isched_);

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
        // std::cout<<"#read refs="<<read_refs_.size()<<"\n";
        // if(write_ref_) {
        //     std::cout<<"WRITE REF FOUND\n";
        // }
        gather_card(S_, R_, W_);
        assert(read_refs_.size() == read_ref_cards_.size());
        construct_read_ref_macros();
        construct_read_dim_maps();
        construct_read_dim_macro_args();
        construct_write_ref_macro();
        construct_write_dim_maps();
        construct_write_dim_macro_args();
        construct_sinstance_macros();
        //islw::destruct(R, W, isched, S);
    }

    ~Statement() {
        islw::destruct(R_, W_, isched_, S_);
        islw::destruct(domain_);
        for(auto rr: read_refs_) {
            islw::destruct(rr);
        }
        read_refs_.clear();
        for(auto rrc: read_ref_cards_) {
            islw::destruct(rrc);
        }
        read_ref_cards_.clear();
        for(auto &rdm: read_dim_maps_) {
          for(auto mp: rdm) {
            islw::destruct(mp);
          }
        }
        read_dim_maps_.clear();

        islw::destruct(write_ref_);
        islw::destruct(write_ref_card_);
        for(auto &wdm: write_dim_maps_) {
            islw::destruct(wdm);
        }
    }

    std::string name() const {
        return std::string{"_s"} + std::to_string(stmt_id_);
    }

    static std::string inline_checks(
      std::vector<Statement*>& stmts,
      std::vector<std::vector<std::string>>& stmtVecItersList)  {
        assert(stmts.size() == stmtVecItersList.size());
      int n = stmts.size();
      std::string ret = "\n{\n";
      std::vector<std::string> vars, inits;
      for(int i=0; i<n; i++) {
        vars.push_back("_d"+std::to_string(i));
      }
      for(const auto& v : vars) {
        inits.push_back(v+"=0");
      }
      ret += "int "+join(inits, ",")+";\n";
      for(int i=0; i<n; i++) {
        ret += stmts[i]->inline_checks(stmtVecItersList[i], vars[i]);
      }
      ret += "_diff |="+join(vars, "*")+";\n";
      ret += "}\n";
      return ret;
    }

    void construct_sinstance_macros() {
        isl_map* stmt_to_refs = nullptr;
        for(auto rr : read_refs_) {
            // std::cerr << "^^RR:" << islw::to_string(rr) << "\n";
            if(stmt_to_refs == nullptr) {
                stmt_to_refs = islw::copy(rr);
                // std::cerr << "^^^S2R update:" << islw::to_string(stmt_to_refs)
                //           << "\n";
            } else {
                stmt_to_refs =
                  isl_map_flat_range_product(stmt_to_refs, islw::copy(rr));
                // std::cerr << "^^^S2R update:" << islw::to_string(stmt_to_refs)
                //           << "\n";
            }
        }
        if(write_ref_) {
            // std::cerr << "^^WR:" << islw::to_string(write_ref_) << "\n";
            if(stmt_to_refs == nullptr) {
                stmt_to_refs = islw::copy(write_ref_);
                // std::cerr << "^^^S2R update:" << islw::to_string(stmt_to_refs)
                //           << "\n";
            } else {
                stmt_to_refs = isl_map_flat_range_product(
                  stmt_to_refs, islw::copy(write_ref_));
                // std::cerr << "^^^S2R update:" << islw::to_string(stmt_to_refs)
                //           << "\n";
            }
        }
        std::vector<std::string> ref_args;
        for(size_t r = 0; r < read_refs_.size(); r++) {
            for(auto d = 0; d < array_sizes_[r]; d++) {
                ref_args.push_back(read_ref_dim_string(r, d));
            }
        }
        if(write_ref_) {
            for(auto d = 0; d < write_array_size_; d++) {
                ref_args.push_back(write_ref_dim_string(d));
            }
        }

        // std::cerr << "S2R before adding version number relations:"
        //           << islw::to_string(stmt_to_refs) << "\n";
        // add version number expressions if they are affine
        assert(stmt_to_refs != nullptr);
        assert(read_ref_cards_.size() == read_refs_.size());
        for(size_t r = 0; r < read_ref_cards_.size(); r++) {
            isl_union_pw_qpolynomial* upw = islw::copy(read_ref_cards_[r]);
            std::string tmp_str          = islw::to_string(upw);
            tmp_str = replace_all(tmp_str, "-> ", "-> [");
            tmp_str = replace_all(tmp_str, ":", "]:");
            tmp_str = replace_all(tmp_str, "-> [{", "-> {");
                // std::cerr << "^^^^^^^^^TRYING VERSION STRING for string="
                //           << tmp_str << "\n";
            isl_map* imap =
              isl_map_read_from_str(islw::context(upw), tmp_str.c_str());
            if(imap != nullptr) {
                // imap = isl_map_set_tuple_name(imap, isl_dim_in, "");
                // std::cerr << "current stmt_to_refs="
                //           << islw::to_string(stmt_to_refs) << "\n";
                stmt_to_refs = isl_map_flat_range_product(stmt_to_refs, imap);
                ref_args.push_back(read_ref_src_string(r));
            } else {
                // std::cerr << "^^^^^^^^^SKIPPING VERSION STRING for string="
                //           << tmp_str << "\n";
            }
            islw::destruct(upw);
        }
#if 0
      //SK: excluding write version numbers from computing statement instance to enable redundant writes
        // add write version number expressions if it is affine
        if(write_ref_card_) {
            isl_union_pw_qpolynomial* upw = islw::copy(write_ref_card_);
            std::string tmp_str          = islw::to_string(upw);
            tmp_str = replace_all(tmp_str, "-> ", "-> [");
            tmp_str = replace_all(tmp_str, ":", "]:");
            tmp_str = replace_all(tmp_str, "-> [{", "-> {");
                std::cerr << "^^^^^^^^^TRYING WVERSION STRING for string="
                          << tmp_str << "\n";
            isl_map* imap =
              isl_map_read_from_str(islw::context(upw), tmp_str.c_str());
            if(imap != nullptr) {
                // imap = isl_map_set_tuple_name(imap, isl_dim_in, "");
                std::cerr << "current stmt_to_refs="
                          << islw::to_string(stmt_to_refs) << "\n";
                stmt_to_refs = isl_map_flat_range_product(stmt_to_refs, imap);
                ref_args.push_back(write_ref_src_string());
            } else {
                std::cerr << "^^^^^^^^^SKIPPING WVERSION STRING for string="
                          << tmp_str << "\n";
            }
            islw::destruct(upw);            
        }
#endif
        //@todo @fixme Also add write version numbers for completeness
        isl_map* refs_to_stmt = isl_map_reverse(stmt_to_refs);
        // if(write_ref_) {
        //     stmt_to_refs =
        //       isl_map_flat_range_product(stmt_to_refs, islw::copy(write_ref_));
        // }

        // std::cerr << "&&&&&&&|" << islw::to_string(refs_to_stmt) << "|\n";
        int ndim = dim();
        for(int i=0; i<ndim; i++) {
            isl_map* r2si = islw::copy(refs_to_stmt);
            if(i<ndim-1) {
                r2si =
                  isl_map_project_out(r2si, isl_dim_out, i + 1, ndim - 1 - i);
            }
            if(i > 0) { r2si = isl_map_project_out(r2si, isl_dim_out, 0, i); }
            // std::cerr << "&&&&&&&-----|" << islw::to_string(r2si) << "|\n";
            if(isl_map_is_single_valued(r2si) != isl_bool_true) {
                std::cerr << __FUNCTION__ << " : " << __LINE__ << ":\n"
                          << "Error: map is not single valued. may not be "
                             "convertible to pwaff for dimension "
                          << i << ". Exiting\n";
                exit(-1);
            }
#if 0
            auto pre = isl_map_set_tuple_name(islw::copy(r2si), isl_dim_out, "");
            std::cerr << "&&&&&&&---pre_upw--|" << islw::to_string(pre)
                      << "|\n";
            std::cerr << "&&&&&&&---pre_upw_umap--|"
                      << islw::to_string(
                           isl_union_map_from_map(islw::copy(pre)))
                      << "|\n";
            std::cerr << "&&&&&&&---pre_upw_umap_1--|"
                      << islw::to_string(isl_union_map_coalesce(
                           isl_union_map_from_map(islw::copy(pre))))
                      << "|\n";
#else
            //@bug @fixme working around an issue with ISL conversion from
            //union_map to pw_aff. Error manifests with adi in polybench
            auto pre =
              isl_map_set_tuple_name(islw::copy(r2si), isl_dim_out, "");
            pre = isl_map_reset_tuple_id(pre, isl_dim_in);
            pre = isl_map_reset_tuple_id(pre, isl_dim_out);
            char* cstr = isl_map_to_str(pre);
            // std::cerr << "&&&&&&&---pre upw--|" << cstr << "|\n";
            // islw::destruct(pre);
            // pre = isl_map_read_from_str(islw::context(domain_), cstr);
            free(cstr);
#endif
            auto upma = isl_union_pw_multi_aff_from_union_map(
              isl_union_map_from_map(pre));
            // char *cstr = isl_union_pw_multi_aff_to_str(upma);
            // std::cerr << "&&&&&&&---upw cstr--|" << cstr << "|\n";
            // free(cstr);
            upma = isl_union_pw_multi_aff_reset_user(upma);
            std::string tmp_str = islw::to_string(upma);
            // std::cerr << "&&&&&&&---upw--|" << tmp_str << "|\n";
            auto pwa =
              isl_pw_aff_read_from_str(islw::context(r2si), tmp_str.c_str());
            if(pwa == nullptr) {
                std::cerr<<__FUNCTION__<<" : "<<__LINE__<<":\n"
                <<"Error: unable to create pw_aff for dimension "<<i<<". Exiting\n";
                exit(-1);
            }
            // std::cerr<<"&&&&&&&---pw--|"<<islw::to_string(pwa)<<"|\n";
            // std::cerr<<"&&&&&&&---pw(c)--|"<<islw::to_c_string(pwa)<<"|\n";
            sinstance_macro_exprs_.push_back(std::string{"("}+islw::to_c_string(pwa)+")");

            auto tmp_sd       = isl_space_domain(isl_pw_aff_get_space(pwa));
            std::string iname = join(iter_names(tmp_sd), ",");
            islw::destruct(tmp_sd);
            sinstance_macro_params_.push_back("(" + iname + ")");

            std::vector<std::string> args = ref_args;
            sinstance_macro_args_.push_back(std::string{"("} + join(args, ",") +
                                            ")");

            islw::destruct(upma);
            islw::destruct(pwa);
            islw::destruct(r2si);
        }

        islw::destruct(refs_to_stmt);
    }

    std::string read_ref_macro_defs() const {
        std::string ret;
        // assert(read_refs_.size() == read_ref_macro_names_.size());
        assert(read_refs_.size() == read_ref_macro_args_.size());
        assert(read_refs_.size() == read_ref_macro_exprs_.size());
        // std::cerr<<"NUM DEF MACROS="<<read_refs_.size()<<"\n";
        for(size_t i = 0; i < read_refs_.size(); i++) {
            // std::cerr<<"name[i]="<<read_ref_macro_name(i)<<"\n";
            // std::cerr<<"args[i]="<<read_ref_macro_args_[i]<<"\n";
            // std::cerr<<"exprs[i]="<<read_ref_macro_exprs_[i]<<"\n";
            // ret += "#define " + read_ref_macro_name(i) +
            //        read_ref_macro_args_[i] + "\t("+
            //        read_ref_macro_exprs_[i] + ")\n";
            ret += read_ref_macro_def_string(i);
        // std::cerr<<"DEFS=\n"<<ret<<"\n";
        }
        // std::cerr<<"DEFS=\n"<<ret<<"\n";
        return ret;
    }

    std::string read_ref_macro_undefs() const {
        std::string ret;
        // assert(read_refs_.size() == read_ref_macro_names_.size());
        for(size_t i = 0; i < read_refs_.size(); i++) {
            //ret += "#undef " + read_ref_macro_name(i) + "\n";
            ret += read_ref_macro_undef_string(i);
        }
        //std::cerr<<"UNDEFS=\n"<<ret<<"\n";
        return ret;
    }

    std::string inline_check_template() const {
      ArrayPack array_pack_{R_, W_};
        std::string str;
        const std::string diff_var = "[[_diff]]";
        const std::string sname = name();
        assert(read_refs_.size() == array_sizes_.size());

        str = "{\n";
        if(write_ref_) {
            for(auto j = 0; j < write_array_size_; j++) {
                std::stringstream ss;
                ss << "int " << write_ref_dim_string(j) << +" = "
                
                   << write_dim_id_macro_name(j) << "([[_" << sname << "_w_d"
                   << j << "]]);\n";
                str += ss.str();
            }
        }
        for(size_t i = 0; i < read_refs_.size(); i++) {
            for(auto j = 0; j < array_sizes_[i]; j++) {
              std::stringstream ss;
              ss << "int " << read_ref_dim_string(i, j) << " = "
                 << read_ref_macro_name(i) << "([[_" + sname + "_r" << i << "_d"
                 << j << "]]);\n";
              str += ss.str();
            }
        }

        str += "\n}\n";
        return str;
    }

    std::string inline_checks(std::vector<std::string> stmtVecIters, 
      const std::string& diff_var="_diff") const {
        std::string str   = "{\n";
        std::string sname = name();
        assert(read_refs_.size() == array_sizes_.size());
        assert(read_refs_.size() == read_array_names_.size());

        auto si = 0;

        if(write_ref_) {
            for(auto j = 0; j < write_array_size_; j++) {
                str += "int " + write_ref_dim_string(j)+
                       " = " + stmtVecIters[si++] +
                       ";\n";
            }
        }
        for(size_t i = 0; i < read_refs_.size(); i++) {
            for(auto j=0; j<array_sizes_[i]; j++) {
                str += "int " +  read_ref_dim_string(i, j) +
                //  sname + "__" + std::to_string(i) + "__" +
                //        std::to_string(j) + 
                       " = " + stmtVecIters[si++]+";\n";
            }
        }
        str += "\n";
        for(size_t i = 0; i < read_refs_.size(); i++) {
            str += read_ref_macro_def_string(i);
        }
        for(size_t i = 0; i < read_refs_.size(); i++) {
            for(auto j = 0; j < array_sizes_[i]; j++) {
                str += read_dim_id_macro_def_string(i, j);
            }
        }
        str += write_ref_macro_def_string();
        for(auto j = 0; j < write_array_size_; j++) {
            str += write_dim_id_macro_def_string(j);
        }
        str += "\n";
        for(auto i = 0; i < dim(); i++) {
            str += sinstance_macro_decl_string(i);
        }
        str += "\n";
        str += sinstance_args_decl_string() + "\n";
        for(size_t i = 0; i < read_refs_.size(); i++) {
            for(auto j = 0; j < array_sizes_[i]; j++) {
                str += diff_var+" |= " + read_dim_id_macro_name(i, j) +
                       "(" + sinstance_args_string() + ")" + 
                       " ^ " +
                       read_ref_dim_string(i, j) +
                       ";\n";
            }
        }
        str += "\n";
        for(auto i = 0U; i < read_refs_.size(); i++) {
            str += diff_var + " |= " + read_ref_macro_name(i) + "(" +
                   sinstance_args_string() + ")" +
                   //  sname + "__" + std::to_string(i) + "(...)" +
                   " ^ " + "(int)(" + read_ref_string(i) + ");\n";
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
#if 1
        for(auto j = 0; j < write_array_size_; j++) {
            str   += "_diff |= " + write_dim_id_macro_name(j) + "(" +
                   sinstance_args_string() + ")" + " ^ " +
                   write_ref_dim_string(j) + ";\n";
        }
        str += "\n";
#if 0
        //SK: comment out check for write version number to enable data layout transformations
        str += diff_var + " |= " + write_ref_macro_name() + "(" +
               sinstance_args_string() + ")" +
               " ^ " + "(int)(" + write_ref_string() + ");\n";
        str += "\n";
#endif
#endif
        if(write_ref_ != nullptr) {
            // str += write_array_name_;
            // if(write_array_size_ != 0) {
            //     str += "[" + sname + "__w__" + std::to_string(0);
            // }
            // for(auto j = 1; j < write_array_size_; j++) {
            //     str += "][" + sname + "__w__" + std::to_string(j);
            // }
            // if(write_array_size_ != 0) { str += "]"; }
            str += "if("+diff_var+"==0) {\n"+
              write_ref_string() +
              #if 1
              //SK: store version number rather than counter
               " = " + write_ref_macro_name() + "(" +
               sinstance_args_string() + ")+1;"+
               #else
              //SK:  store counter
              " += 1;" + 
              #endif
              "\n}\n";
        }
        str += "\n";
        for(auto i = 0; i < dim(); i++) {
            str += sinstance_macro_undef_string(i);
        }
        str += "\n";
        for(size_t i = 0; i < read_refs_.size(); i++) {
            for(auto j = 0; j < array_sizes_[i]; j++) {
                str += read_dim_id_macro_undef_string(i, j);
            }
        }
        for(auto i = 0U; i < read_refs_.size(); i++) {
            str += read_ref_macro_undef_string(i);
        }
        for(auto j = 0; j < write_array_size_; j++) {
            str += write_dim_id_macro_undef_string(j);
        }
        str += write_ref_macro_undef_string();
        str += "\n} /*"+diff_var+"*/\n";
        return str;
    }

    std::vector<std::string> stmt_varids() const {
      return stmt_varids_;
    }

    private:

    /**
     * @bug @fixme Is this the same as read_ref_string()?
     */
    std::string read_ref_src_string(int read_ref_id) const {
        assert(read_ref_id >= 0);
        assert(read_ref_id < read_refs_.size());
        std::string name{
          isl_map_get_tuple_name(read_refs_[read_ref_id], isl_dim_out)};
        std::vector<std::string> arg_names;
        assert(array_sizes_.size() == read_refs_.size());
        for(int i=0; i<array_sizes_[read_ref_id]; i++) {
            arg_names.push_back(read_ref_dim_string(read_ref_id, i));
        }
        if(!arg_names.empty()) {
            return name + "[" + join(arg_names, "][") + "]";
        } else {
            return name;
        }
    }

    std::string write_ref_src_string() const {
        assert(write_ref_ != nullptr);
        std::string name{
          isl_map_get_tuple_name(write_ref_, isl_dim_out)};
        std::vector<std::string> arg_names;
        assert(array_sizes_.size() == read_refs_.size());
        for(int i=0; i<write_array_size_; i++) {
            arg_names.push_back(write_ref_dim_string(i));
        }
        if(!arg_names.empty()) {
            return name + "[" + join(arg_names, "][") + "]";
        } else {
            return name;
        }
    }

    std::string read_ref_dim_string(int read_ref_id, int dim_id) const {
        return name() + "_r" + std::to_string(read_ref_id) + "_" +
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
        return name() + "_w_" + std::to_string(dim_id);
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

    std::string read_ref_macro_name(int read_ref_id) const {
        return std::string{"PC_V_S"} + std::to_string(stmt_id_) + "_r" +
               std::to_string(read_ref_id);
    }
    std::string write_ref_macro_name() const {
        return std::string{"PC_V_S"} + std::to_string(stmt_id_) + "_w";
    }

    std::string read_ref_macro_undef_string(int read_ref_id) const {
        return "#undef " + read_ref_macro_name(read_ref_id)+"\n";
    }
    std::string write_ref_macro_undef_string() const {
        return "#undef " + write_ref_macro_name()+"\n";
    }

    std::string sinstance_dim_string(int id) const {
      assert(id>=0);
      assert(id < dim());
      return name() + "_i"+std::to_string(id);
    }

    std::string sinstance_args_decl_string() const {
        std::string ret;
        for(int i = 0; i < dim(); i++) {
            ret += "int " + sinstance_dim_string(i) + " = " +
                   sinstance_macro_name(i) + 
                    sinstance_macro_args_string(i)+";\n";
        }
        return ret;
    }

    std::string sinstance_args_string() const {
        std::vector<std::string> args;
        for(int i=0; i<dim(); i++) {
            args.push_back(sinstance_dim_string(i));
        }
        return join(args, ",");
    }

    std::string sinstance_macro_name(int dim_id) const {
        return std::string{"PC_I_S"} + std::to_string(stmt_id_) + "_" +
               std::to_string(dim_id);
    }

    std::string sinstance_macro_params_string(int dim_id) const {
        assert(dim_id >=0 && dim_id< dim());
        return sinstance_macro_params_[dim_id];
    }

    std::string sinstance_macro_args_string(int dim_id) const {
        assert(dim_id >=0 && dim_id< dim());
        return sinstance_macro_args_[dim_id];
    }

    std::string sinstance_macro_decl_string(int dim_id) const {
        assert(dim_id >=0 && dim_id< dim());
        return "#define " + sinstance_macro_name(dim_id) +
               sinstance_macro_params_string(dim_id) + "\t" +
               sinstance_macro_exprs_[dim_id]+"\n";
            //    "/*to be filled by sriram*/\n";
    }

    std::string sinstance_macro_undef_string(int dim_id) const {
        return "#undef " + sinstance_macro_name(dim_id) + "\n";
    }

    std::string read_dim_id_macro_name(int read_ref_id, int dim_id) const {
      assert(read_ref_id >=0);
      assert(read_ref_id < read_refs_.size());
      assert(read_ref_id < array_sizes_.size());
      assert(dim_id >= 0 && dim_id < array_sizes_[read_ref_id]);
      return "PC_I_S" + std::to_string(stmt_id_) + "_r" +
             std::to_string(read_ref_id) + "_i" + std::to_string(dim_id);
    }
    std::string write_dim_id_macro_name(int dim_id) const {
        assert(dim_id >= 0 && dim_id < write_array_size_);
        return "PC_I_S" + std::to_string(stmt_id_) + "_w" + "_i" +
               std::to_string(dim_id);
    }
    std::string read_dim_id_macro_def_string(int read_ref_id,
                                             int dim_id) const {
        assert(read_ref_id >= 0);
        assert(read_ref_id < read_dim_macro_args_.size());
        assert(domain_ != nullptr);
        assert(dim_id < read_dim_macro_args_[read_ref_id].size());

        isl_map* newm = isl_map_set_tuple_name(
          islw::copy(read_dim_maps_[read_ref_id][dim_id]), isl_dim_out, "");
        std::cerr << islw::to_string(newm) << "\n";
        std::string str = islw::to_string(newm);
        isl_pw_aff* pwa =
          isl_pw_aff_read_from_str(islw::context(domain_), str.c_str());
        islw::destruct(newm);

        std::string ret;
#if 0
      ret = "#if defined(" + read_dim_id_macro_name(read_ref_id, dim_id) +
             ")\n" +
             "#error \"Polycheck error : macro name conflict.Try a different\
          prefix (not yet supported).\" \n" +
             "#endif\n";
#endif
    //   std::cerr << "READ_DIM_ID_DIM_STRING. stmt=" << stmt_id_
    //             << " read_ref=" << read_ref_id << " dim_id=" << dim_id << "\n";
    //             std::cerr<<"STRING = |"<<str<<"| \n";
      ret += "#define " + read_dim_id_macro_name(read_ref_id, dim_id) + "(" +
             read_dim_macro_args_[read_ref_id][dim_id] + ")\t" +
             //  islw::to_c_string(read_dim_maps_[read_ref_id][dim_id])

             //  islw::to_c_string(
             //    isl_union_pw_multi_aff_from_union_map(isl_union_map_from_map(
             //      islw::copy(read_dim_maps_[read_ref_id][dim_id]))))
             "(" + islw::to_c_string(pwa) + ")\n";
      islw::destruct(pwa);
      return ret;
    }

    std::string write_dim_id_macro_def_string(int dim_id) const {
        assert(domain_ != nullptr);
        assert(dim_id < write_dim_macro_args_.size());

        isl_map* newm = isl_map_set_tuple_name(
          islw::copy(write_dim_maps_[dim_id]), isl_dim_out, "");
        std::cerr << islw::to_string(newm) << "\n";
        std::string str = islw::to_string(newm);
        isl_pw_aff* pwa =
          isl_pw_aff_read_from_str(islw::context(domain_), str.c_str());
        islw::destruct(newm);

        std::string ret;
    //   std::cerr << "WRITE_DIM_ID_DIM_STRING. stmt=" << stmt_id_
    //             << " write_ref=0" << " dim_id=" << dim_id << "\n";
    //             std::cerr<<"STRING = |"<<str<<"| \n";
      ret += "#define " + write_dim_id_macro_name(dim_id) + "(" +
             write_dim_macro_args_[dim_id] + ")\t" +
             //  islw::to_c_string(read_dim_maps_[read_ref_id][dim_id])

             //  islw::to_c_string(
             //    isl_union_pw_multi_aff_from_union_map(isl_union_map_from_map(
             //      islw::copy(read_dim_maps_[read_ref_id][dim_id]))))
             "(" + islw::to_c_string(pwa) + ")\n";
      islw::destruct(pwa);
      return ret;
    }

    std::string read_dim_id_macro_undef_string(int read_ref_id,
                                               int dim_id) const {
        return "#undef " + read_dim_id_macro_name(read_ref_id, dim_id) + "\n";
    }

    std::string write_dim_id_macro_undef_string(int dim_id) const {
        return "#undef " + write_dim_id_macro_name(dim_id) + "\n";
    }

    std::string read_ref_macro_def_string(int read_ref_id) const {
      assert(read_ref_id >=0);
      assert(read_ref_id < read_refs_.size());
      assert(read_ref_id < read_ref_macro_args_.size());
      assert(read_ref_id < read_ref_macro_exprs_.size());
      std::string ret;
#if 0
      ret =
        "#if defined(" + read_ref_macro_name(read_ref_id) + ")\n" +
        "#error \"Polycheck error : macro name conflict.Try a different\
          prefix (not yet supported).\" \n" +
        "#endif\n";
#endif
      ret += "#define " + read_ref_macro_name(read_ref_id) +
             read_ref_macro_args_[read_ref_id] + "\t(" +
             read_ref_macro_exprs_[read_ref_id] + ")\n";
      return ret;
    }

    std::string write_ref_macro_def_string() const {
        std::string ret;
        ret += "#define " + write_ref_macro_name() +write_ref_macro_args_ +
               "\t(" + write_ref_macro_exprs_ + ")\n";
        return ret;
    }

    /**
     * @brief Gatherr read and write references, array names, 
     * and number of array dimensions.
     */
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
                isl_set* tmp_set = isl_set_from_union_set(
                  isl_union_map_range(pet_expr_access_get_may_read(expr)));
                array_sizes_.push_back(isl_set_dim(tmp_set, isl_dim_set));
                islw::destruct(tmp_set);
                // std::cout<<"&&&& DIM "<<read_array_names_.back()<<" = "<<array_sizes_.back()<<"\n";
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
                isl_set* tmp_set = isl_set_from_union_set(
                  isl_union_map_range(pet_expr_access_get_may_write(expr)));
                write_array_size_ = isl_set_dim(tmp_set,
                  isl_dim_set);
                islw::destruct(tmp_set);
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
     * write_card_ = card S . (((S^-1) . write_ref_ .(TW^-1)) * LT )
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
        isl_union_set_lex_gt_union_set(islw::copy(T), islw::copy(T));
        isl_union_map* LE =
        //   isl_union_map_from_map(isl_map_lex_lt(isl_union_set_get_space(T)));
        isl_union_set_lex_ge_union_set(islw::copy(T), islw::copy(T));
        isl_union_map* TWinv =
          isl_union_map_reverse(islw::umap_compose(Sinv, W));

        for(size_t i=0; i<read_refs_.size(); i++) {
            isl_union_map* rr_umap =
              isl_union_map_from_map(islw::copy(read_refs_[i]));
            isl_union_map* T_to_prevW = isl_union_map_intersect(
              islw::copy(LT), islw::umap_compose(Sinv, rr_umap, TWinv));
            isl_union_map* Tself =
              isl_union_set_identity(isl_union_map_range(islw::umap_compose_left(
                isl_union_map_reverse(islw::copy(rr_umap)), S)));
            T_to_prevW = isl_union_map_union(T_to_prevW, islw::copy(Tself));
            islw::destruct(Tself);
            islw::destruct(rr_umap);
            isl_union_map* S_to_prevW = islw::umap_compose(S, T_to_prevW);
            read_ref_cards_.push_back(isl_union_map_card(islw::copy(S_to_prevW)));
            // std::cerr<<"---------------------------\n";
            // std::cerr << "STMT " << stmt_id_ << " READ " << i<<"\n";
            // std::cerr<<"read_refs:\n"<<islw::to_string(read_refs_[i])<<"\n";
            // std::cerr<<"LT:\n"<<islw::to_string(LT)<<"\n";
            auto* Tspc = isl_union_set_get_space(T);
            // std::cerr<<"Space(T): "<<islw::to_string(Tspc)<<"\n";
            islw::destruct(Tspc);
            // std::cerr<<"T:\n"<<islw::to_string(T)<<"\n";
            // std::cerr<<"TWinv:\n"<<islw::to_string(TWinv)<<"\n";
            // std::cerr<<"Sinv:\n"<<islw::to_string(Sinv)<<"\n";
            // std::cerr<<"T_to_prevW:\n"<<islw::to_string(T_to_prevW)<<"\n";
            // std::cerr<<"S_to_prevW:\n"<<islw::to_string(S_to_prevW)<<"\n";

            // std::cerr              << " CARD:\n-----\n";

            std::cout<<islw::to_string(read_ref_cards_.back())<<"\n------\n";

            auto pwqp = isl_union_pw_qpolynomial_extract_pw_qpolynomial(
                          read_ref_cards_.back(),
                           isl_union_pw_qpolynomial_get_space(
                             read_ref_cards_.back()));
            // std::cout << "pw\n-------\n"
            //           << isl_pw_qpolynomial_to_str(pwqp)
            //           << "\n----\n";
                      islw::destruct(pwqp);
            // std::cout<<islw::to_c_string(isl_union_pw_qpolynomial_to_polynomial()
            // read_ref_cards_.back())<<"\n------\n";
            islw::destruct(S_to_prevW);
            islw::destruct(T_to_prevW);
        }
        if(write_ref_) {
            isl_union_map* wr_umap =
              isl_union_map_from_map(islw::copy(write_ref_));
            isl_union_map* T_to_prevW = isl_union_map_intersect(
              islw::copy(LT), islw::umap_compose(Sinv, wr_umap, TWinv));
            isl_union_map* Tself =
              isl_union_set_identity(isl_union_map_range(islw::umap_compose_left(
                isl_union_map_reverse(islw::copy(wr_umap)), S)));
            T_to_prevW = isl_union_map_union(T_to_prevW, islw::copy(Tself));
            islw::destruct(Tself);
            islw::destruct(wr_umap);
            isl_union_map* S_to_prevW = islw::umap_compose(S, T_to_prevW);
            write_ref_card_ = isl_union_map_card(islw::copy(S_to_prevW));
            islw::destruct(S_to_prevW);
            islw::destruct(T_to_prevW);
        }
        islw::destruct(Sinv, sinstances, T, LT, LE, TWinv);
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

            auto tmp_sd = isl_space_domain(isl_map_get_space(read_refs_[i]));
            std::string iname = join(iter_names(tmp_sd), ",");
            islw::destruct(tmp_sd);
            read_ref_macro_args_.push_back("(" + iname + ")");
            // std::cerr << "RR MACRO= #define " << read_ref_macro_name(i)
            //           << "     "<< read_ref_macro_args_.back() 
            //           <<"    "<<islw::to_string(read_ref_cards_[i])<<"\n";
        }
        construct_read_ref_macro_exprs();
    }

    void construct_write_ref_macro() {
        std::string str;
        assert(write_ref_);
        auto tmp_sd       = isl_space_domain(isl_map_get_space(write_ref_));
        std::string iname = join(iter_names(tmp_sd), ",");
        islw::destruct(tmp_sd);
        write_ref_macro_args_ = "(" + iname + ")";
        // std::cerr << "WR MACRO= #define " << write_ref_macro_name() << "     "
        //           << write_ref_macro_args_ << "    "
        //           << islw::to_string(write_ref_card_) << "\n";
        construct_write_ref_macro_exprs();
    }

    void construct_read_dim_macro_args() {
        for(size_t i = 0; i < read_refs_.size(); i++) {
            read_dim_macro_args_.push_back({});
            for(int j = 0; j < array_sizes_[i]; j++) {
                auto tmp_sd =
                  isl_space_domain(isl_map_get_space(read_dim_maps_[i][j]));
                std::string iname = join(iter_names(tmp_sd), ",");
                islw::destruct(tmp_sd);
                read_dim_macro_args_.back().push_back(iname);
            }
        }
    }

    void construct_write_dim_macro_args() {
        for(int j = 0; j < write_array_size_; j++) {
            auto tmp_sd =
              isl_space_domain(isl_map_get_space(write_dim_maps_[j]));
            std::string iname = join(iter_names(tmp_sd), ",");
            islw::destruct(tmp_sd);
            write_dim_macro_args_.push_back(iname);
        }
    }

    void construct_read_dim_maps() {
        assert(read_refs_.size() == array_sizes_.size());
        for(size_t i = 0; i < read_refs_.size(); i++) {
            read_dim_maps_.push_back({});
            for(int j = 0; j < array_sizes_[i]; j++) {
                isl_map* rm = islw::copy(read_refs_[i]);
                if(j < array_sizes_[i] - 1) {
                    rm = isl_map_project_out(rm, isl_dim_out, j + 1,
                                             array_sizes_[i] - 1 - j);
                }
                if(j > 0) { rm = isl_map_project_out(rm, isl_dim_out, 0, j); }
                read_dim_maps_.back().push_back(rm);
            }
        }
    }

    void construct_write_dim_maps() {
        assert(write_ref_);
        for(int j = 0; j < write_array_size_; j++) {
            isl_map* rm = islw::copy(write_ref_);
            if(j < write_array_size_ - 1) {
                rm = isl_map_project_out(rm, isl_dim_out, j + 1,
                                         write_array_size_ - 1 - j);
            }
            if(j > 0) { rm = isl_map_project_out(rm, isl_dim_out, 0, j); }
            write_dim_maps_.push_back(rm);
        }
    }

    void construct_read_ref_macro_exprs() {
        for(size_t i=0; i<read_ref_cards_.size(); i++) {
            // read_ref_macro_exprs_.push_back(islw::to_string(read_ref_cards_[i]));
            std::string str;
            isl_union_pw_qpolynomial_foreach_pw_qpolynomial(
              read_ref_cards_[i], print_pw_qpoly, &str);
            if(str.empty()) { str = std::to_string(0); }
            for(int c=0; c<=9; c++) {
                str = replace_all(str, " " + std::to_string(c),
                                  " (int64_t)" + std::to_string(c));
            }
            read_ref_macro_exprs_.push_back(str);
        }        
    }

    void construct_write_ref_macro_exprs() {
        std::string str;
        isl_union_pw_qpolynomial_foreach_pw_qpolynomial(write_ref_card_,
                                                        print_pw_qpoly, &str);
        if(str.empty()) { str = std::to_string(0); }
        for(int c = 0; c <= 9; c++) {
            str = replace_all(str, " " + std::to_string(c),
                              " (int64_t)" + std::to_string(c));
        }
        write_ref_macro_exprs_ = str;
    }

    int dim() const {
      assert(domain_);
      return isl_set_n_dim(domain_);
    }

    isl_union_map* R_;
    isl_union_map* W_;
    isl_schedule* isched_;
    isl_union_map* S_;
    int stmt_id_;     // id of the statement from PET
    isl_set* domain_; //statement domain
    std::vector<isl_map*> read_refs_; //maps to read array references
    std::vector<std::string> read_array_names_; //names of all array refs on the RHS
    std::vector<int> array_sizes_; //dimensionality of the i-th read array reference 
    std::vector<isl_union_pw_qpolynomial*> read_ref_cards_; //
    // std::vector<std::string> read_ref_macro_names_;
    std::vector<std::string> read_ref_macro_args_;
    std::vector<std::string> read_ref_macro_exprs_;
    std::vector<std::vector<isl_map*>> read_dim_maps_;
    std::vector<std::vector<std::string>> read_dim_macro_args_;
    //std::vector<std::string> inline_checks_;

    isl_map* write_ref_; //map to the write array reference
    std::string write_array_name_; //name of the array written
    int write_array_size_; //dimensionality of the write array reference
    isl_union_pw_qpolynomial* write_ref_card_;
    std::string write_ref_macro_args_;
    std::string write_ref_macro_exprs_;
    std::vector<std::string> stmt_varids_;
    std::vector<std::string> sinstance_macro_exprs_;
    std::vector<std::string> sinstance_macro_params_;
    std::vector<std::string> sinstance_macro_args_;
    std::vector<isl_map*> write_dim_maps_;
    std::vector<std::string> write_dim_macro_args_;
    // isl_union_map* sched_;
}; // class Statement 
