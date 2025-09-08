#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

#define CLPARSE_IMPLEMENTATION
#define NO_USE_WIDE_ARGV
#include "clparse.h"

// defer in C
#include "cefer.h"

#define UNUSED(_val) (void)(_val)

// cleanups
static void cleanClparse(void* unused) {
    UNUSED(unused);
    clparseDeinit();
}

static void cleanCtx(void* ctx_p) {
    fz_context* ctx = ctx_p;
    if (ctx) fz_drop_context(ctx);
}

// ex: 3-5,8,10-12
const int* parseRange(const char* range_str, int page_count, int* count) {
    int cap = 32, n = 0;
    int* output = malloc(sizeof(int) * cap);
    bool cleanup = false;
    DEFER_IF(&cleanup, free, output);

    const char* ptr = range_str;

    while (*ptr) {
        while (isspace(*ptr) || *ptr == ',') ++ptr;
        if (!*ptr) break;

        char* end_ptr;
        long start = strtol(ptr, &end_ptr, 10);
        if (end_ptr == ptr || start <= 0 || start > page_count) {
            cleanup = true;
            return NULL;
        }

        ptr = end_ptr;
        int end = start;
        if (*ptr == '-') {
            end = strtol(++ptr, &end_ptr, 10);
            if (end_ptr == ptr || end <= 0 || end < start) {
                cleanup = true;
                return NULL;
            }
            ptr = end_ptr;
        }

        for (int k = start; k <= end; ++k) {
            if (n == cap)
                output = realloc(output, sizeof(int) * (cap <<= 1));
            output[n++] = k - 1;
        }
    }

    *count = n;
    return output;
}

int main(int argc, char** argv) {
    clparseInit("pdfutils", "PDF utilities");
    DEFER(cleanClparse, NULL);

    bool* subpdf = clparseSubcmd("subpdf", "Extract sub-PDF");
    const char** in_path = clparseMainArg("IN_PATH", "asdasd", "subpdf");
    const char** range = clparseMainArg("RANGE", "asdasd", "subpdf");
    const char** out_path = clparseStr("output", 'o', "output.pdf",
        "output filename", "subpdf");

    if (!clparseParse(argc, argv)) {
        fprintf(stderr, "ERROR: parsing commandline failed\n");
        return 1;
    }

    if (clparseIsHelp()) {
        clparsePrintHelp();
        return 0;
    }

    if (!*subpdf) {
        fprintf(stderr, "ERROR: %s\n", clparseGetErr());
        clparsePrintHelp();
        return 1;
    }

    fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx) {
        fprintf(stderr, "ERROR: failed initializing fz_context\n");
        return 1;
    }
    DEFER(cleanCtx, ctx);

    fz_document* doc = NULL;
    pdf_document* src = NULL;
    pdf_document* dst = NULL;

    fz_try(ctx) {
        fz_register_document_handlers(ctx);

        doc = fz_open_document(ctx, *in_path);
        if (!doc) fz_throw(ctx, FZ_ERROR_GENERIC, "cannot open document %s", *in_path);

        src = pdf_specifics(ctx, doc);
        if (!src) fz_throw(ctx, FZ_ERROR_GENERIC, "%s is not a PDF", *in_path);

        int page_count = pdf_count_pages(ctx, src);
        int n_idx  = 0;
        const int* idx = parseRange(*range, page_count, &n_idx);
        if (!idx || n_idx == 0) {
            free((void*)idx);
            fz_throw(ctx, FZ_ERROR_GENERIC, "bad page range or empty");
        }

        dst = pdf_create_document(ctx);
        if (!dst) {
            free((void*)idx);
            fz_throw(ctx, FZ_ERROR_GENERIC, "cannot create empty PDF");
        }

        for (int i = 0; i < n_idx; ++i) {
            pdf_graft_page(ctx, dst, i, src, idx[i]);
        }

        pdf_save_document(ctx, dst, *out_path, NULL);
    }
    fz_always(ctx) {
        if (dst) pdf_drop_document(ctx, dst);
        if (src) pdf_drop_document(ctx, src);
        if (doc) fz_drop_document(ctx, doc);
    }
    fz_catch(ctx) {
        const char* msg = fz_caught_message(ctx);
        fprintf(stderr, "ERROR: %s\n", msg ? msg : "(unknown)");
    }

    printf("Wrote sub-PDF: %s\n", *out_path);
    return 0;
}
