#ifndef JDAW_AUTO_PAGE_H
#define JDAW_AUTO_PAGE_H

#include "api.h"
#include "endpoint.h"
#include "page.h"
#include "window.h"


typedef struct auto_page_els {
    Endpoint *ep;
    const char *label_if_different;
    enum slider_style slider_style;
} AutoPageEl;

void auto_page(Page *page, AutoPageEl *els, int num_els);


#endif
