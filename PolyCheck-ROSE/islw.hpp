#ifndef PolyCheck_islw_hpp_
#define PolyCheck_islw_hpp_

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

#include <barvinok/isl.h>

/**
 * @brief C++ convenience wrappers to commonly used ISL routines
 */
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
isl_ctx* context(isl_union_pw_multi_aff* obj) {
    return isl_union_pw_multi_aff_get_ctx(obj);
}

template<>
isl_ctx* context(isl_pw_aff* obj) {
    return isl_pw_aff_get_ctx(obj);
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
isl_printer* to_printer(isl_printer* printer, isl_union_pw_multi_aff* obj) {
    return isl_printer_print_union_pw_multi_aff(printer, obj);
}

template<>
isl_printer* to_printer(isl_printer* printer, isl_pw_aff* obj) {
    return isl_printer_print_pw_aff(printer, obj);
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
void destruct(isl_val* obj) {
    if(obj) {
        isl_val_free(obj);
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
void destruct(isl_union_pw_multi_aff* obj) {
    if(obj) {
        isl_union_pw_multi_aff_free(obj);
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
void destruct(isl_pw_aff* obj) {
    if(obj) {
        isl_pw_aff_free(obj);
    }
}

template<>
void destruct(isl_space* obj) {
    if(obj) {
        isl_space_free(obj);
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

#endif // PolyCheck_islw_hpp_
