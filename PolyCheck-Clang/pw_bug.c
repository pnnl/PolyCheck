#include <assert.h>

#include <pet.h>
#include <isl/aff.h>
#include <isl/map.h>

int main() {
    //const char* str_bug = "[tsteps, n] -> { [i0, i1, i0, -1 + i1, i0, i1, i0, -1 + i1, i0, i1, i10, 1 + i10, i10] -> [i2 = i1] : 0 <= i0 < n and 2 <= i1 <= 1023 and i1 < n and 0 <i10 < tsteps }";
    //const char* str_bug = "[tsteps, n] -> { [i0, i1, i0, -1 + i1] -> [i2 = i1] : 0 <= i0 < n and 2 <= i1 <= 1023 and i1 < n }";
    const char* str_bug = "[tsteps, n] -> { [i0, i1, i0] -> [i2 = i1] : 0 <= i0 < n and 2 <= i1 <= 1023 and i1 < n }";

    isl_ctx* ctx;
    isl_union_map *umap;
    isl_union_pw_multi_aff *pwma1, *pwma2;
    char *pwma1_str, *pwma2_str;

    ctx = isl_ctx_alloc();

    umap = isl_union_map_read_from_str(ctx, str_bug);
    assert(umap != NULL); //passes
    pwma1 = isl_union_pw_multi_aff_from_union_map(umap);
    assert(pwma1 != NULL); //passes
    pwma1_str = isl_union_pw_multi_aff_to_str(pwma1);
    assert(pwma1_str != NULL); //passes but the string seems invalid
    printf("PWMA1:%s\n", pwma1_str);

    pwma2 = isl_union_pw_multi_aff_read_from_str(ctx, pwma1_str); //fails
    assert(pwma2 != NULL); //fails because pwma1_str is invalid

    free(pwma1_str);
    isl_union_pw_multi_aff_free(pwma1);
    isl_union_pw_multi_aff_free(pwma2);
    isl_ctx_free(ctx);
    return 0;
}