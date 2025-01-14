/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software iso
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/


/*****************************************************************************************************************
    project_endpoint_ops.h

    * project-level functions managing operations related to the Endpoints API
 *****************************************************************************************************************/


#include "project.h"

int project_queue_val_change(Project *proj, Endpoint *ep, Value new_val);
void project_flush_val_changes(Project *proj, enum jdaw_thread thread);

int project_queue_callback(Project *proj, Endpoint *ep, EndptCb cb, enum jdaw_thread thread);
void project_flush_callbacks(Project *proj, enum jdaw_thread thread);

int project_add_ongoing_change(Project *proj, Endpoint *ep, enum jdaw_thread thread);
void project_do_ongoing_changes(Project *proj, enum jdaw_thread thread);
void project_flush_ongoing_changes(Project *proj, enum jdaw_thread thread);
