//#include "polycheck-runtime.h"

#include <string>
#include <iostream>
#include <regex>
#include <cassert>
#include <set>
#include <map>

#include <isl/ctx.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <isl/set.h>
#include <isl/map.h>
#include <isl/printer.h>
#include <isl/point.h>
#include <isl/id.h>
#include <isl/aff.h>
#include <isl/polynomial.h>

#include <pet.h>
#include <barvinok/isl.h>

#include "testInsert.hpp"
//using namespace polycheck;

//const char* smap = "[ni, nj] -> { C[i0, i1] : i0 >= 0 and i0 < ni and i1 >= 0 and i1 < nj and i1 <= 1023 }";

// const char* wstr = "[ni, nj] -> { S_0[i, j] -> C[o0, o1] : o0 = i and o1 = j and i >= 0 and i < ni and j >= 0 and j < nj and j <= 1023 }";

// const char *rstr = "[ni, nj] -> {  }";

#if 0
template<typename T>
isl_ctx* context(T* obj);

template<>
isl_ctx* context(isl_union_map* umap) {
    return isl_union_map_get_ctx(umap);
}

template<>
isl_ctx* context(isl_point* point) {
    return isl_point_get_ctx(point);
}

template<typename T>
isl_printer* to_printer(isl_printer* printer, T* obj);

template<>
isl_printer* to_printer(isl_printer* printer, isl_union_map* obj) {
    return isl_printer_print_union_map(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_point* point) {
    return isl_printer_print_point(printer, point);
}

template<typename T>
void destruct(T* obj);

template<>
void destruct(isl_id *id) {
    isl_id_free(id);
}

template<>
void destruct(isl_point *point) {
    isl_point_free(point);
}

template<>
void destruct(isl_union_set* uset) {
    isl_union_set_free(uset);
}

template<>
void destruct(isl_union_map* umap) {
    isl_union_map_free(umap);
}

template<>
void destruct(isl_ctx* ctx) {
    isl_ctx_free(ctx);
}

template<typename T>
T* copy(T* obj);

template<>
isl_union_map* copy(isl_union_map* umap) {
    return isl_union_map_copy(umap);
}

template<>
isl_union_set* copy(isl_union_set* uset) {
    return isl_union_set_copy(uset);
}


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

template<typename T>
std::string to_c_string(T* obj) {
    return to_string(obj, ISL_FORMAT_C);
}


int main() {
    //polycheck_init();
    //polycheck_statement_add("S_0",wstr, rstr);
    //polycheck_finalize();
    isl_ctx* ctx = isl_ctx_alloc();

    isl_union_map* umap = isl_union_map_read_from_str(ctx, wstr);
    std::cout<<"umap="<<to_string(umap)<<"\n";

    unsigned nparam = isl_union_map_dim(umap, isl_dim_param);
    std::cout<<"umap ndim=:"<<nparam<<"\n";
    std::string fixed_dim_wstr{wstr};
    for(unsigned int i=0;i<nparam; i++) {
        isl_id* id = isl_union_map_get_dim_id(umap, isl_dim_param, i);
        std::string name{isl_id_get_name(id)};
        std::cout<<"umap param["<<i<<"] = "<<name<<"\n";
        std::string pattern = "\\b(" + name+")\\b";// + "\\([:punct:]\\)";
        // std::string repl = std::string{"(\\1)"} + "25" + "(\\2)";
        //std::string pattern {name};
        std::string repl = "25";
        std::cout<<"pattern: "<<pattern<<"\n";
        std::cout<<"pattern: "<<repl<<"\n";
        std::regex rpattern{pattern};
        fixed_dim_wstr = std::regex_replace(fixed_dim_wstr, rpattern,repl);
        std::cout<<"modified wstr="<<fixed_dim_wstr<<"\n";
        isl_id_free(id);
    }
    isl_union_map* fixed_umap;
    {
        std::regex remove_params {"^[[:space:]]*\\[[a-zA-Z0-9,\\s]*\\]"};
        fixed_dim_wstr = std::regex_replace(fixed_dim_wstr, remove_params, "[ ]");
        std::cout<<"final str="<<fixed_dim_wstr<<"\n";

        fixed_umap = isl_union_map_read_from_str(ctx, fixed_dim_wstr.c_str());
        std::cout << "new umap=" << to_string(fixed_umap) << "\n";
    }
    {
        isl_union_set* uset = isl_union_map_domain(copy(fixed_umap));
        auto fn             = [](isl_point* point, void* user) -> isl_stat {
            std::cout<<to_string(point)<<"\n";
            destruct(point);
            return isl_stat_ok;
        };
        std::cout<<"Points:\n";
        isl_union_set_foreach_point(uset, fn, nullptr);
        destruct(uset);
    }
    destruct(fixed_umap);
    destruct(umap);
    destruct(ctx);
    return 0;
}
#endif

// void test_c_workflow() {
//     polycheck::Checker checker;

//     polycheck_init();

//     polycheck_program_spec();

//     polycheck_shedule_spec();

//     polycheck_param_spec();

//     polycheck_array_loc_register();

//     polycheck_check_start();

//     polycheck_check_operation(wptr, r1ptr, r2ptr);

//     polycheck_check_last_write();

//     polycheck_check_end();

//     polychek_finalize();
// }


namespace impl {
isl_union_map* compute_bounded_map_from_str(
  isl_ctx* ctx, const std::string& map_str,
  const std::map<std::string, int>& param_values) {
    isl_union_map* umap = isl_union_map_read_from_str(ctx, map_str.c_str());
    int nparam          = param_values.size();
    assert(nparam == isl_union_map_dim(umap, isl_dim_param));
    std::string fixed_dim_str = islw::to_string(umap);
    for(auto i = 0; i < nparam; i++) {
        isl_id* id = isl_union_map_get_dim_id(umap, isl_dim_param, i);
        std::string name{isl_id_get_name(id)};
        assert(param_values.find(name) != param_values.end());
        // std::cout << "umap param[" << i << "] = " << name << "\n";
        std::string pattern = "\\b(" + name + ")\\b"; // + "\\([:punct:]\\)";
        std::string repl    = std::to_string(param_values.find(name)->second);
        std::regex rpattern{pattern};
        fixed_dim_str = std::regex_replace(fixed_dim_str, rpattern, repl);
        // std::cout << "modified wstr=" << fixed_dim_str << "\n";
        isl_id_free(id);
    }
    std::regex remove_params{"^[[:space:]]*\\[[a-zA-Z0-9,\\s]*\\]"};
    fixed_dim_str = std::regex_replace(fixed_dim_str, remove_params, "[ ]");
    std::cout << "final spec str=" << fixed_dim_str << "\n";

    islw::destruct(umap);
    return isl_union_map_read_from_str(ctx, fixed_dim_str.c_str());
}
} // namespace internal

class ArrayInfo {
    public:
    ArrayInfo() = default;
    ArrayInfo(const std::string& name, int dim) : name_{name}, dim_{dim} {}
    const std::string& name() const { return name_; }
    int dim() const { return dim_; }

    private:
    std::string name_;
    int dim_;
};

/**
 * @brief 
 * 
 * @note Looks like we don't need write_dep, but maybe for another algorithm.
 * 
 */
class StmtInfo {
    public:
    StmtInfo() = default;
    StmtInfo(const std::string& name, isl_union_map* write_ref,
             std::vector<isl_union_map*>&& read_refs,
             isl_union_map* stmt_writes_, isl_union_map* schedule_) :
      name_{name},
      write_ref_{islw::copy(write_ref)},
      read_refs_{read_refs} {
        construct_read_deps(stmt_writes_, schedule_);
    }
    ~StmtInfo() {
        islw::destruct(write_ref_);
        for(auto& umap : read_refs_) { islw::destruct(umap); }
        read_refs_.clear();
        for(auto& umap : read_deps_) { islw::destruct(umap); }
        read_deps_.clear();
    }
    const std::string& name() const { return name_; }
    int arity() const { return read_refs_.size(); }

    isl_union_set* read_ref(isl_union_set* sinstance, int pos) const {
        assert(sinstance != nullptr);
        assert(pos >= 0);
        assert(pos < arity());
        return islw::apply_map(read_refs_[pos], sinstance);
    }

    isl_union_set* read_dep(isl_union_set* sinstance, int pos) const {
        assert(sinstance != nullptr);
        assert(pos >= 0);
        assert(pos < arity());
        return islw::apply_map(read_deps_[pos], sinstance);
    }

    private:
    /**
     * @brief 
     * 
     * Given:
     * - S: Statement to time
     * - R[i]: i-th read in this statement
     * - W : Statement to write (for all statements)
     * 
     * - T = S^-1
     * - WT = (T . W)^-1
     * - GT = (dom T) >> (dom T)
     * - ReadDep[i] = lexmax (T . R[i] . (WT)) * GT
     */
    void construct_read_deps(isl_union_map* stmt_writes,
                             isl_union_map* schedule) {
        isl_union_map* S = schedule;
        isl_union_map* W = stmt_writes;
        isl_union_map* T  = isl_union_map_reverse(islw::copy(S));
        isl_union_map* WT = isl_union_map_reverse(islw::umap_compose(T, W));
        isl_union_map* GT = isl_union_map_from_map(isl_map_lex_gt(
          isl_union_set_get_space(isl_union_map_domain(islw::copy(T)))));
        for(isl_union_map* Ri : read_refs_) {
            isl_union_map* RDi = isl_union_map_intersect(
              islw::copy(GT), islw::umap_compose(T, Ri, WT));
            read_deps_.push_back(isl_union_map_lexmax(RDi));
        }
        islw::destruct(S);
        islw::destruct(W);
        islw::destruct(WT);
        islw::destruct(GT);
    }

    std::string name_;
    isl_union_map* write_ref_; //mostly unused
    std::vector<isl_union_map*> read_refs_;
    std::vector<isl_union_map*> read_deps_;
};

class Instance {
    public:
    Instance() : array_loc(nullptr), cur_writer(nullptr) {}

    Instance(isl_union_set* aloc) :
      array_loc{islw::copy(aloc)},
      cur_writer{nullptr} {}
    ~Instance() {
        islw::destruct(array_loc);
        islw::destruct(cur_writer);
    }
    isl_union_set* array_loc;
    isl_union_set* cur_writer;
}; // class Instance

class Program {
    public:
    Program(isl_ctx* ctx) 
    : ctx_{ctx} {
        stmt_reads_   = nullptr;
        stmt_writes_  = nullptr;
        schedule_     = nullptr;
        first_writer_ = nullptr;
        last_writer_  = nullptr;
        next_writer_  = nullptr;
        reads_        = nullptr;
        writes_       = nullptr;
        reads_writes_ = nullptr;
        // ctx_          = nullptr;
    }

    void cleanup() {
        islw::destruct(stmt_reads_);
        islw::destruct(stmt_writes_);
        islw::destruct(schedule_);
        islw::destruct(first_writer_);
        islw::destruct(last_writer_);
        islw::destruct(next_writer_);
        islw::destruct(reads_);
        islw::destruct(writes_);
        islw::destruct(reads_writes_);

        arrays_.clear();
        statements_.clear();
        // islw::destruct(ctx_);

        // stmt_reads_   = nullptr;
        // stmt_writes_  = nullptr;
        // schedule_     = nullptr;
        // first_writer_ = nullptr;
        // last_writer_  = nullptr;
        // next_writer_  = nullptr;
        // reads_        = nullptr;
        // writes_       = nullptr;
        // reads_writes_ = nullptr;
        // ctx_          = nullptr;
    }

    ~Program() {
        cleanup();
    }

    Program(const Program& pgm) :
      param_values_{pgm.param_values_},
      stmt_reads_spec_{pgm.stmt_reads_spec_},
      stmt_writes_spec_{pgm.stmt_writes_spec_},
      schedule_spec_{pgm.schedule_spec_},
      stmt_refs_spec_{pgm.stmt_refs_spec_},
      fw_spec_{pgm.fw_spec_},
      lw_spec_{pgm.lw_spec_},
      nw_spec_{pgm.nw_spec_},
      rw_spec_{pgm.rw_spec_},
      stmt_reads_{islw::copy(pgm.stmt_reads_)},
      stmt_writes_{islw::copy(pgm.stmt_writes_)},
      schedule_{islw::copy(pgm.schedule_)},
      first_writer_{islw::copy(pgm.first_writer_)},
      last_writer_{islw::copy(pgm.last_writer_)},
      next_writer_{islw::copy(pgm.next_writer_)},
      reads_{islw::copy(pgm.reads_)},
      writes_{islw::copy(pgm.writes_)},
      reads_writes_{islw::copy(pgm.reads_writes_)},
      ctx_{pgm.ctx_},
      statements_{pgm.statements_},
      arrays_{pgm.arrays_} {}

    Program& operator = (Program&& pgm) {
        std::swap(*this, pgm);
        return *this;
    }

    // void context(isl_ctx* ctx) {
    //     assert(ctx != nullptr);
    //     assert(ctx_ == nullptr);
    //     ctx_ = ctx;
    // }

    isl_ctx* context() {
        assert(ctx_ != nullptr);
        return ctx_;
    }

    void stmt_reads_spec(const std::string& reads_spec) {
        assert(!reads_spec.empty());
        stmt_reads_spec_ = reads_spec;
    }

    void stmt_writes_spec(const std::string& writes_spec) {
        assert(!writes_spec.empty());
        stmt_writes_spec_ = writes_spec;
    }

    void schedule_spec(const std::string& schedule_spec) {
        assert(!schedule_spec.empty());
        schedule_spec_ = schedule_spec;
    }

    void first_writer_spec(const std::string& fw_spec) {
        assert(!fw_spec.empty());
        fw_spec_ = fw_spec;
    }

    void last_writer_spec(const std::string& lw_spec) {
        assert(!lw_spec.empty());
        lw_spec_ = lw_spec;
    }

    void next_writer_spec(const std::string& nw_spec) {
        assert(!nw_spec.empty());
        nw_spec_ = nw_spec;
    }

    void reads_writes_spec(const std::string& rw_spec) {
        assert(!rw_spec.empty());
        rw_spec_ = rw_spec;
    }

    void param_spec(const std::string& param_name, int param_value) {
        assert(param_values_.find(param_name) == param_values_.end());
        assert(param_value >=0);
        param_values_[param_name] = param_value;
    }

    void array_spec(const std::string& array_name, int ndim) {
        assert(arrays_.find(array_name) == arrays_.end());
        arrays_[array_name] = ArrayInfo{array_name, ndim};
    }

    void stmt_spec(const std::string& stmt_name,
                   const std::vector<std::string>& array_refs_spec) {
        assert(stmt_refs_spec_.find(stmt_name) == stmt_refs_spec_.end());
        stmt_refs_spec_[stmt_name] = array_refs_spec;
    }

    // isl_union_map* compute_bounded_map(isl_union_map* umap,
    //     const std::map<std::string, int>& param_values) const {
    //     isl_ctx* ctx = islw::context(umap);
    //     int nparam = param_values.size();
    //     assert(nparam == isl_union_map_dim(umap, isl_dim_param));
    //     std::string fixed_dim_str = islw::to_string(umap);
    //     for(unsigned int i = 0; i < nparam; i++) {
    //         isl_id* id = isl_union_map_get_dim_id(umap, isl_dim_param, i);
    //         std::string name{isl_id_get_name(id)};
    //         assert(param_values.find(name) != param_values.end());
    //         //std::cout << "umap param[" << i << "] = " << name << "\n";
    //         std::string pattern =
    //           "\\b(" + name + ")\\b"; // + "\\([:punct:]\\)";
    //         std::string repl = std::to_string(param_values.find(name)->second);
    //         std::regex rpattern{pattern};
    //         fixed_dim_str = std::regex_replace(fixed_dim_str, rpattern, repl);
    //         //std::cout << "modified wstr=" << fixed_dim_str << "\n";
    //         isl_id_free(id);
    //     }
    //     std::regex remove_params{"^[[:space:]]*\\[[a-zA-Z0-9,\\s]*\\]"};
    //     fixed_dim_str = std::regex_replace(fixed_dim_str, remove_params, "[ ]");
    //     std::cout << "final spec str=" << fixed_dim_str << "\n";

    //     return isl_union_map_read_from_str(ctx, fixed_dim_str.c_str());
    // }

 
    void construct_bounded_spec() {
        assert(ctx_ != nullptr);
        assert(!stmt_reads_spec_.empty());
        assert(!stmt_writes_spec_.empty());
        assert(!schedule_spec_.empty());
        stmt_reads_ =
          impl::compute_bounded_map_from_str(ctx_, stmt_reads_spec_, param_values_);
        stmt_writes_ =
          impl::compute_bounded_map_from_str(ctx_, stmt_writes_spec_, param_values_);
        schedule_ =
          impl::compute_bounded_map_from_str(ctx_, schedule_spec_, param_values_);
    }

    void construct_reads_and_writes() {
        assert(!stmt_reads_spec_.empty());
        assert(!stmt_writes_spec_.empty());
        assert(ctx_ != nullptr);
        reads_ = isl_union_map_range(
          isl_union_map_read_from_str(ctx_, stmt_reads_spec_.c_str()));
        writes_ = isl_union_map_range(
          isl_union_map_read_from_str(ctx_, stmt_writes_spec_.c_str()));
        reads_writes_ =
          isl_union_set_union(islw::copy(reads_), islw::copy(writes_));
    }

    /**
     * @brief Construct first writer relation
     * 
     * Given:
     * - W: map from statement to location written
     * - S: map from statement to time point
     * 
     * FirstWriter = lexmin (((S^-1).W)^-1)
     */
    void construct_first_writer() {
        assert(stmt_writes_ != nullptr);
        assert(schedule_ != nullptr);

        isl_union_map* S = schedule_;
        isl_union_map* W = stmt_writes_;
        isl_union_map* Sinv = isl_union_map_reverse(islw::copy(S));
        isl_union_map* Sinv_dot_W = islw::umap_compose_left(Sinv, islw::copy(W));
        first_writer_ = isl_union_map_lexmin(isl_union_map_reverse(Sinv_dot_W));
        //std::cout<<"first_writer_ = "<<islw::to_string(first_writer_)<<"\n";
    }

    /**
     * @brief Construct last writer relation
     * 
     * Given:
     * - W: map from statement to location written
     * - S: map from statement to time point
     * 
     * LastWriter = lexmax (((S^-1).W)^-1)
     */
    void construct_last_writer() {
        assert(stmt_writes_ != nullptr);
        assert(schedule_ != nullptr);

        isl_union_map* S = schedule_;
        isl_union_map* W = stmt_writes_;
        isl_union_map* Sinv = isl_union_map_reverse(islw::copy(S));
        isl_union_map* Sinv_dot_W = islw::umap_compose_left(Sinv, W);
        last_writer_ = isl_union_map_lexmax(isl_union_map_reverse(Sinv_dot_W));
        std::cout<<"last_writer_ = "<<islw::to_string(last_writer_)<<"\n";
    }

    /**
     * @brief Construct first writer relation
     * 
     * Given:
     * - W: map from statement to location written
     * - S: map from statement to time point
     * 
     * Steps:
     * - T = range(S)
     * - LT = T << T
     * - TW = (S^-1) . W
     * - NextWriter = lexmin ((TW . (TW^-1)) * LT)
     */
    void construct_next_writer() {
        assert(stmt_writes_ != nullptr);
        assert(schedule_ != nullptr);

        isl_union_map* S = schedule_;
        isl_union_map* W = stmt_writes_;
        isl_union_set* T = isl_union_map_range(islw::copy(S));
        isl_union_map* LT =
          isl_union_map_from_map(isl_map_lex_lt(isl_union_set_get_space(T)));
        isl_union_map* TW =
          islw::umap_compose_left(isl_union_map_reverse(islw::copy(S)), W);

        next_writer_ = isl_union_map_lexmax(isl_union_map_intersect(
          isl_union_map_apply_range(TW, isl_union_map_reverse(islw::copy(TW))),
          LT));
        islw::destruct(T);
        std::cout << "next_writer_ = " << islw::to_string(next_writer_) << "\n";
    }

    void validate() {
        //todo implement

        //we know spec for all statements and arrays in the spec

        //every statement has a schedule
    }

    void construct_statement_info() {
        for(const auto& stmt : stmt_refs_spec_) {
            std::vector<isl_union_map*> array_refs;
            for(const auto& ref_string : stmt.second) {
                array_refs.push_back(impl::compute_bounded_map_from_str(
                  ctx_, ref_string, param_values_));
            }
            statements_[stmt.first] = StmtInfo{
              stmt.first, nullptr, std::move(array_refs), stmt_writes_, schedule_};
        }
    }

    void build() {
        validate();
        assert(ctx_ != nullptr);
        // ctx_ = isl_ctx_alloc();
        construct_bounded_spec();

        // todo: bypass some steps when user provides additional inputs. for
        // now, all state is constructed from program_spec and schedule_spec

        // build remaining isl objects
        construct_reads_and_writes();
        construct_statement_info();
        construct_first_writer();
        construct_last_writer();
        construct_next_writer();
    }
    isl_union_set* first_writer(isl_union_set* uset) {
        return islw::apply_map(first_writer_, uset);
    }
    isl_union_set* last_writer(isl_union_set* uset) {
        return islw::apply_map(last_writer_, uset);
    }
    isl_union_set* next_writer (isl_union_set* uset) {
        return islw::apply_map(next_writer_, uset);
    }

    isl_union_map* first_writer() {
        return islw::copy(first_writer_);
    }
    isl_union_map* last_writer() {
        return islw::copy(last_writer_);
    }
    isl_union_map* next_writer() {
        return islw::copy(next_writer_);
    }

    isl_union_set* reads() {
        return islw::copy(reads_);
    }
    isl_union_set* writes() {
        return islw::copy(writes_);
    }
    isl_union_set* reads_writes() {
        return islw::copy(reads_writes_);
    }

    // class ArrayLocInfo;
    // const std::map<void*,ArrayLocInfo>& array_locations() const {
    //     return array_locations_;
    // }

    const StmtInfo& statement(const std::string& stmt_name) const {
        auto it = statements_.find(stmt_name);
        assert(it != statements_.end());
        return it->second;
    }

    const ArrayInfo& array(const std::string& array_name) const {
        auto it = arrays_.find(array_name);
        assert(it != arrays_.end());
        return it->second;
    }

    private:
    std::map<std::string, int> param_values_;
    std::string stmt_reads_spec_;
    std::string stmt_writes_spec_;
    std::string schedule_spec_;
    std::map<std::string, std::vector<std::string>> stmt_refs_spec_;
    std::string fw_spec_;
    std::string lw_spec_;
    std::string nw_spec_;
    std::string rw_spec_;

    //std::map<void*,ArrayLocInfo> array_locations_;

    isl_union_map* stmt_reads_;
    isl_union_map* stmt_writes_;
    isl_union_map* schedule_;
    isl_union_map* first_writer_;
    isl_union_map* last_writer_;
    isl_union_map* next_writer_;
    isl_union_set* reads_;
    isl_union_set* writes_;
    isl_union_set* reads_writes_;
    isl_ctx* ctx_; //not owned

    std::map<std::string,StmtInfo> statements_;
    std::map<std::string,ArrayInfo> arrays_;

    // friend void swap(Program& first, Program& second) {
    //     using std::swap;

    //     swap(first.param_values_, second.param_values_);
    //     swap(first.stmt_reads_spec_, second.stmt_reads_spec_);
    //     swap(first.stmt_writes_spec_, second.stmt_writes_spec_);
    //     swap(first.schedule_spec_, second.schedule_spec_);
    //     swap(first.fw_spec_, second.fw_spec_);
    //     swap(first.lw_spec_, second.lw_spec_);
    //     swap(first.nw_spec_, second.nw_spec_);
    //     swap(first.rw_spec_, second.rw_spec_);

    //     swap(first.stmt_reads_, second.stmt_reads_);
    //     swap(first.stmt_writes_, second.stmt_writes_);
    //     swap(first.schedule_, second.schedule_);
    //     swap(first.first_writer_, second.first_writer_);
    //     swap(first.last_writer_, second.last_writer_);
    //     swap(first.next_writer_, second.next_writer_);
    //     swap(first.reads_, second.reads_);
    //     swap(first.writes_, second.writes_);
    //     swap(first.reads_writes_, second.reads_writes_);
    //     swap(first.ctx_, second.ctx_);
    //     swap(first.statements_, second.statements_);
    //     swap(first.arrays_, second.arrays_);
    // }
}; // class Program

class PolyCheck {
    public:
    PolyCheck(isl_ctx* ctx) : ctx_{ctx}, program_{ctx} {
        // ctx_ = isl_ctx_alloc();
        // program_.context(ctx_);
    }

    void program_stmt_reads_spec(const std::string& reads_spec) {
        program_.stmt_reads_spec(reads_spec);
    }

    void program_stmt_writes_spec(const std::string& writes_spec) {
        program_.stmt_writes_spec(writes_spec);
    }

    void schedule_spec(const std::string& schedule_spec) {
        program_.schedule_spec(schedule_spec);
    }

    void first_writer_spec(const std::string& fw_spec) {
        program_.first_writer_spec(fw_spec);
    }

    void last_writer_spec(const std::string& lw_spec) {
        program_.last_writer_spec(lw_spec);
    }

    void next_writer_spec(const std::string& nw_spec) {
        program_.next_writer_spec(nw_spec);
    }

    void reads_writes_spec(const std::string& rw_spec) {
        program_.reads_writes_spec(rw_spec);
    }

    void param_spec(const std::string& param_name, int param_value) {
        program_.param_spec(param_name, param_value);
    }

    void array_spec(const std::string& array_name, int ndim) {
        program_.array_spec(array_name, ndim);
    }

    void stmt_spec(const std::string& stmt_name,
                   const std::vector<std::string>& array_refs_spec) {
        program_.stmt_spec(stmt_name, array_refs_spec);
    }

    void spec_complete() {
        program_.build();
        auto fn = [](isl_point* point, void* user) -> isl_stat {
            std::set<std::string>& unbound_set =
              *static_cast<std::set<std::string>*>(user);
            std::string str = islw::to_string(point);
            assert(unbound_set.find(str) == unbound_set.end());
            unbound_set.insert(str);
            islw::destruct(point);
            return isl_stat_ok;
        };
        isl_union_set* rw = program_.reads_writes();
        isl_union_set_foreach_point(rw, fn,
                                    &unbound_array_locs_);
        islw::destruct(rw);
    }

    std::string array_index_to_string(const std::string& array_name,
                                      const std::vector<int>& idx) {
        // const ArrayInfo& ainfo = program_.array(array_name);
        // assert(ainfo.dim() == idx.size());
        std::string str = array_name + "[";
        for(int i = 0; i < -1 + (int)idx.size(); i++) {
            str += std::to_string(idx[i]) + ", ";
        }
        if(idx.size() > 0) { str += std::to_string(idx.back()); }
        str = "{" + str + "]}";
        auto uset = isl_union_set_read_from_str(program_.context(), str.c_str());
        str = islw::to_string(uset);
        islw::destruct(uset);
        return str;
    }

    void array_loc_register(void* array_loc, const std::string& array_name,
                            const std::vector<int>& idx) {
        assert(array_loc != nullptr);
        const ArrayInfo& ainfo = program_.array(array_name);
        assert(ainfo.dim() == idx.size());
        std::string aname = array_index_to_string(array_name, idx);
        assert(unbound_array_locs_.find(aname) != unbound_array_locs_.end());
        assert(data_map_.find(array_loc) == data_map_.end());
        isl_union_set* uset  = isl_union_set_read_from_str(program_.context(), aname.c_str());
        data_map_[array_loc] = Instance{uset};
        islw::destruct(uset);
        unbound_array_locs_.erase(aname);
    }

    void check_begin() { assert(unbound_array_locs_.empty()); }

    void check_operation(const std::vector<std::string>& stmt_matches,
                         void* write_ptr, const std::vector<void*>& read_ptrs) {
        assert(write_ptr != nullptr);
        assert(data_map_.find(write_ptr) != data_map_.end());

        const Instance& wloc = data_map_[write_ptr];
        isl_union_set* this_stmt;
        if(wloc.cur_writer == nullptr) {
            this_stmt = program_.first_writer(wloc.array_loc);
        } else {
            this_stmt = program_.next_writer(wloc.cur_writer);
        }
        const char* sname = isl_basic_set_get_tuple_name(
          isl_basic_set_from_point(isl_union_set_sample_point(this_stmt)));
        std::string stmt_name{sname};
        free(const_cast<char*>(sname));

        assert(std::find(stmt_matches.begin(), stmt_matches.end(), stmt_name) !=
               stmt_matches.end());

        const StmtInfo& sinfo = program_.statement(stmt_name);
        assert(sinfo.arity() == read_ptrs.size());
        for(auto i = 0; i < sinfo.arity(); i++) {
            assert(read_ptrs[i] != nullptr);
            assert(data_map_.find(read_ptrs[i]) != data_map_.end());
            const Instance& rloc    = data_map_[read_ptrs[i]];
            isl_union_set* writer_i = sinfo.read_dep(this_stmt, i);
            assert(isl_union_set_is_equal(rloc.cur_writer, writer_i) ==
                   isl_bool_true);
        }
        data_map_[write_ptr].cur_writer = this_stmt;
    }

    void check_end() {
        for(const auto& data_entry: data_map_) {
            isl_union_set* loc = data_entry.second.array_loc;
            isl_union_set* cur_writer = data_entry.second.cur_writer;
            isl_union_set* last_writer = program_.last_writer(loc);
            assert(isl_union_set_is_equal(cur_writer, last_writer) == isl_bool_true);
        }
    }

    ~PolyCheck() {
        std::cerr<<__FUNCTION__<<" "<<__LINE__<<"\n";
        //program_ = Program{};
        //data_map_.clear();
        //program_.cleanup();
        std::cerr<<__FUNCTION__<<" "<<__LINE__<<"\n";
        //islw::destruct(ctx_);
        std::cerr<<__FUNCTION__<<" "<<__LINE__<<"\n";
    }

    private:
    isl_ctx* ctx_; //not owned
    std::set<std::string> unbound_array_locs_;
    std::map<void *, Instance> data_map_;
    Program program_;
};

#if 0
typedef void* polycheck_t;

void polycheck_init(polycheck_t* pc);

void polycheck_program_spec(polycheck_t pc, const char* program_spec);

void polycheck_shedule_spec(polycheck_t pc, const char* schecule_spec);

void polycheck_param_spec(polycheck_t pc, const char* param_name,
                          int param_value);

void polycheck_array_loc_register(polycheck_t pc, void* array_loc,
                                  const char* array_name, int ndim, int* idx);

void polycheck_array_loc_register_0d(polycheck_t pc, void* array_loc,
                                     const char* array_name);

void polycheck_array_loc_register_1d(polycheck_t pc, void* array_loc,
                                     const char* array_name, int id0);

void polycheck_array_loc_register_2d(polycheck_t pc, void* array_loc,
                                     const char* array_name, int id0, int id1);

void polycheck_array_loc_register_3d(polycheck_t pc, void* array_loc,
                                     const char* array_name, int id0, int id1,
                                     int id2);

void polycheck_array_loc_register_4d(polycheck_t pc, void* array_loc,
                                     const char* array_name, int id0, int id1,
                                     int id2, int id3);

void polycheck_check_begin(polycheck_t pc);

void polycheck_check_m1_0d(polycheck_t pc, const char* stmt_name, void* wptr);
void polycheck_check_m1_1d(polycheck_t pc, const char* stmt_name, void* wptr,
                           void* r0);
void polycheck_check_m1_2d(polycheck_t pc, const char* stmt_name, void* wptr,
                           void* r0, void* r1);
void polycheck_check_m1_3d(polycheck_t pc, const char* stmt_name, void* wptr,
                           void* r0, void* r1, void* r2);
void polycheck_check_m1_3d(polycheck_t pc, const char* stmt_name, void* wptr,
                           void* r0, void* r1, void* r2, void* r3);
void polycheck_check_m2_0d(polycheck_t pc, const char* s0, const char* s1,
                           void* wptr);
void polycheck_check_m2_1d(polycheck_t pc, const char* s0, const char* s1,
                           void* wptr, void* r0);
void polycheck_check_m2_2d(polycheck_t pc, const char* s0, const char* s1,
                           void* wptr, void* r0, void* r1);
void polycheck_check_m2_3d(polycheck_t pc, const char* s0, const char* s1,
                           void* wptr, void* r0, void* r1, void* r2);
void polycheck_check_m2_3d(polycheck_t pc, const char* s0, const char* s1,
                           void* wptr, void* r0, void* r1, void* r2, void* r3);

void polycheck_check(polycheck_t pc, int nmatches, const char* stmt_names[],
                     void* wptr, int nreads, void* rptrs[]);

void polycheck_check_last_write(polycheck_t pc, void* wptr);

void polycheck_check_end(polycheck_t pc);

void polycheck_finalize(polycheck_t* pc);
#endif

std::string replace_all(const std::string& str_in, const std::string& from,
                        const std::string& to) {
    std::string str{str_in};
    if(!from.empty()) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }
    return str;
}

// void replace_all(std::string& str, const std::string& from, const std::string& to) {
//     if(from.empty())
//         return;
//     size_t start_pos = 0;
//     while((start_pos = str.find(from, start_pos)) != std::string::npos) {
//         str.replace(start_pos, from.length(), to);
//         start_pos += to.length();
//     }
// }



std::string array_ref_string(isl_set* iset) {
    std::string aref{isl_set_get_tuple_name(iset)};
    return aref + "(" + join(iter_names(iset), ", ") + ")";
}

std::string array_ref_string_in_c(isl_set* iset) {
    std::string aref{isl_set_get_tuple_name(iset)};
    std::string idx = join(iter_names(iset), "][");
    if (!idx.empty()) {
        return aref + "[" + idx + "]";
    } else {
        return aref;
    }
}

//should be a lambda
isl_stat fn_prolog_per_set(isl_set* set, void* user) {
    std::string& str   = *(std::string*)user;
    std::string prolog = islw::to_c_string(set);
    std::string array_ref_str = array_ref_string(set) + ";";
    std::string repl_str = array_ref_string_in_c(set) + "=0;";
    str += replace_all(prolog, array_ref_str, repl_str);
    // std::cout<<"REF STR="<<array_ref_str<<"\n";
    // std::cout<<"REPLACEMENT STR="<<repl_str<<"\n";
    islw::destruct(set);
    return isl_stat_ok;
}

std::string prolog(isl_union_map* R, isl_union_map* W) {
    std::string prolog;
    isl_union_set* RW = isl_union_set_union(isl_union_map_range(islw::copy(R)),
                                            isl_union_map_range(islw::copy(W)));
    // std::cerr << "RW elements = ";
    // isl_union_set_dump(RW);
    isl_union_set_foreach_set(RW, fn_prolog_per_set, &prolog);
    prolog = "#include <assert.h>\n int _diff = 0;\n" + prolog + "\n";
    islw::destruct(RW);
    return prolog;
}

//should be a lambda
isl_stat epilog_per_poly_piece(isl_set* set, isl_qpolynomial* poly, void *user) {
  std::string& str = *(std::string*) user;
  std::string set_code = islw::to_c_string(set);
  std::string poly_code = islw::to_c_string(poly);
  std::string array_ref_str = array_ref_string(set) + ";";
  std::string repl_str      = "_diff |= ((int)" + array_ref_string_in_c(set) +
                         ") ^ " + poly_code + ";";
  str += replace_all(set_code, array_ref_str, repl_str);
  islw::destruct(set);
  islw::destruct(poly);
  return isl_stat_ok;
}

//should be a lambda
isl_stat epilog_per_poly(isl_pw_qpolynomial* pwqp, void *user) {
  isl_pw_qpolynomial_foreach_piece(pwqp, epilog_per_poly_piece, user);
  islw::destruct(pwqp);
  return isl_stat_ok;
}

std::string epilog(isl_union_map* W) {
    std::string epilog;
    isl_union_pw_qpolynomial* WC =
      isl_union_map_card(isl_union_map_reverse(islw::copy(W)));
    isl_union_pw_qpolynomial_foreach_pw_qpolynomial(WC, epilog_per_poly,
                                                    &epilog);
    epilog += "\n assert(_diff == 0); \n";
    islw::destruct(WC);
    return epilog;
}

int print_expr(__isl_keep pet_expr *expr, void *user) {
    std::cout << "+++\n";
    std::cout<<"NAME="<<islw::to_string(pet_expr_access_get_id(expr))<<"\n";
    std::cout<<"IDX="<<islw::to_string(pet_expr_access_get_index(expr))<<"\n";
    //std::cout<<"n_arg="<<islw::to_string(pet_expr_access_get_ref_id(expr))<<"\n";
    //pet_expr_dump(expr);
    std::cout << "+++\n";
    return 0;
}



int main(int argc, char* argv[]) {
    // std::string writes = "[ni, nj] -> { S_0[i, j] -> C[o0, o1] : o0 = i and
    // o1 = j and i >= 0 and i < ni and j >= 0 and j < nj and j <= 1023 }";

    // std::string reads = "[ni, nj] -> {  }";

    // std::string schedule = "[ni, nj] -> { S_0[i, j] -> [o0, o1] : o0 = i and
    // o1 = j and i >= 0 and i < ni and j >= 0 and j < nj and j <= 1023 }";

    assert(argc == 2);
    std::string filename{argv[1]};
    std::string target{argv[2]};

     

    struct pet_options* options = pet_options_new_with_defaults();
    isl_ctx* ctx = isl_ctx_alloc_with_options(&pet_options_args, options);
    struct pet_scop* scop =
      pet_scop_extract_from_C_source(ctx, filename.c_str(), NULL);

    isl_union_map *R, *W, *S;
    R = pet_scop_get_may_reads(scop);
    W = pet_scop_get_may_writes(scop);
    isl_schedule* isched = pet_scop_get_schedule(scop);
    S = isl_schedule_get_map(isched);

    //pet_scop_dump(scop);
    // isl_union_map* sched = isl_union_map_intersect (
    //   islw::copy(S), isl_union_map_union(islw::copy(R), islw::copy(W)));

    // std::cerr<<"R:-----\n"<<islw::to_string(R)<<"-----\n";
    // std::cerr<<"W:-----\n"<<islw::to_string(W)<<"-----\n";

    // std::cerr<<"R:-----\n"<<islw::to_c_string(R)<<"-----\n";
    // std::cerr<<"W:-----\n"<<islw::to_c_string(W)<<"-----\n";
    // std::cerr<<"S:-----\n"<<islw::to_string(S)<<"-----\n";
    // std::string reads = islw::to_c_string(R);
    //std::string writes = islw::to_c_string(W);
    //std::string schedule = islw::to_c_string(sched);

#if 0
    {
        std::cerr << __FUNCTION__ << " " << __LINE__ << "\n";
        PolyCheck polycheck{ctx};
        std::cerr << __FUNCTION__ << " " << __LINE__ << "\n";
        polycheck.program_stmt_reads_spec(reads);
        polycheck.program_stmt_writes_spec(writes);
        polycheck.schedule_spec(schedule);
        polycheck.param_spec("ni", 4);
        polycheck.param_spec("nj", 2);
        polycheck.spec_complete();
        polycheck.check_begin();
        polycheck.check_end();
    }
#endif
     std::cout<<"Prolog\n------\n"<<prolog(R, W)<<"\n----------\n";
     std::cout<<"Epilog\n------\n"<<epilog(W)<<"\n----------\n";

     std::cout << "#statements=" << scop->n_stmt << "\n";
     for(int i = 0; i < scop->n_stmt; i++) {
        //std::cout<<"-----------\n";
         //std::cout << "Statement " << i << " nargs=" << scop->stmts[i]->n_arg
                //    << "\n";
        //pet_tree_dump(scop->stmts[i]->body);
        //std::cout<<"---\n";        
        // pet_tree_foreach_access_expr(scop->stmts[i]->body,
        //     print_expr, nullptr);
        //  for(int j = 0; j < scop->stmts[i]->n_arg; j++) {
        //      pet_expr_dump(scop->stmts[i]->args[j]);
        //  }
         //     isl_printer* printer = isl_printer_to_str(ctx);
         //     isl_id_to_ast_expr* i2ae =
         //     pet_stmt_build_ast_exprs(scop->stmt[i],
         //     __isl_keep isl_ast_build *build,
         //     __isl_give isl_multi_pw_aff *(*fn_index)(
         //             __isl_take isl_multi_pw_aff *mpa, __isl_keep isl_id *id,
         //             void *user), void *user_index,
         // __isl_give isl_ast_expr *(*fn_expr)(__isl_take isl_ast_expr *expr,
         //             __isl_keep isl_id *id, void *user), void *user_expr);
         //     //printer = pet_stmt_print_body(scop->stmts[i], printer,
         //     nullptr);
         //     std::cout<<"Statements:"<<isl_printer_get_str(printer)<<"\n";
         //     islw::destruct(printer);
     }
    //  isl_union_map* RW = isl_union_map_union(islw::copy(R), islw::copy(W));
    //  std::map<std::string, isl_union_map*> dict;
    //  isl_union_map_foreach_map(RW, extract_accesses, &dict);
    //  std::cout<<"RW\n-----\n"<<islw::to_string(RW)<<"\n----\n";

    std::string defs, undefs;
    std::vector<std::string> inline_checks;
    std::vector<Statement> stmts;
     {
      
         for(int i = 0; i < scop->n_stmt; i++) {
             stmts.push_back(Statement{i, scop});
         }
         for(const auto& stmt: stmts) {
             defs += stmt.read_ref_macro_defs();
             undefs += stmt.read_ref_macro_undefs();
             inline_checks.push_back(stmt.inline_checks());
         }
     }

     const std::string prologue = prolog(R, W) + "\n" + defs;
     const std::string epilogue = undefs + "\n" + epilog(W);
     std::cout<<"Prolog\n------\n"<< prologue <<"\n----------\n";
     std::cout<<"Epilog\n------\n"<< epilogue <<"\n----------\n";
     std::cout<<"Inline checks\n------\n";
    for(size_t i=0; i<inline_checks.size(); i++) {
        std::cout<<"  Statement "<<i<<"\n  ---\n"<<inline_checks[i]<<"\n";
    }
    std::cout<<"-------\n";

     ParseScop(target, stmts, prologue, epilogue, GetOutputFileName(filename));

     pet_scop_free(scop);
     isl_schedule_free(isched);
     islw::destruct(R, W, S, ctx);
     return 0;
}