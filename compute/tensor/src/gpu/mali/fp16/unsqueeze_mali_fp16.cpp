// Copyright (C) 2019. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "sys.h"
#include "types.h"
#include "error.h"
#include "gpu/mali/fp16/unsqueeze_mali_fp16.h"

inline EE unsqueeze_checkpara_mali_fp16(TensorDesc inputDesc, TensorDesc outputDesc)
{
    if (inputDesc.dt != outputDesc.dt) {
        return NOT_SUPPORTED;
    }
    if (outputDesc.dt != DT_F16) {
        return NOT_SUPPORTED;
    }
    return SUCCESS;
}

inline EE unsqueeze_core_mali_fp16(
    GCLHandle_t handle, TensorDesc inputDesc, GCLMem_t input, TensorDesc outputDesc, GCLMem_t output)
{
    UNUSED(outputDesc);
    U32 iw, ih, ic, in;
    if (inputDesc.df == DF_NCHW) {
        tensorSelectGet(inputDesc, NULL, NULL, &in, &ic, &ih, &iw);
    } else if (inputDesc.df == DF_MKT) {
        get_nlp_mkt_val(inputDesc, NULL, &in, &ic, &ih);
        iw = 1;
    } else {
        return NOT_SUPPORTED;
    }
    U32 iw_str, ih_str, iw_off, ih_off;
    ih_str = input->desc.stride[0];
    iw_str = input->desc.stride[1];
    ih_off = input->desc.offset[0];
    iw_off = input->desc.offset[1];
    U32 ow_str, oh_str, ow_off, oh_off;
    oh_str = output->desc.stride[0];
    ow_str = output->desc.stride[1];
    oh_off = output->desc.offset[0];
    ow_off = output->desc.offset[1];

    cl_mem inbuf, outbuf;
    inbuf = input->mem;
    outbuf = output->mem;

    U32 gs[3] = {ih, iw, (ic + 3) / 4};
    U32 ls[3] = {0, 0, 0};
    U32 dim = 3;
    Kernel kernel;
    CHECK_STATUS(gcl_create_kernel(handle, "squeeze", &kernel));
    CHECK_STATUS(gcl_set_kernelArgs(kernel, ih, iw, ih_str, iw_str, ih_off, iw_off, oh_str, ow_str,
        oh_off, ow_off, inbuf, outbuf));
    gcl_set_kernelVec(handle, kernel, dim, gs, ls, "squeeze");
#ifdef _DEBUG
    CHECK_STATUS(gcl_run_kernel(handle, kernel, dim, gs, ls, "squeeze"));
    CHECK_STATUS(gcl_print_memory<F16>(handle, input, "unsqueeze_input"));
    CHECK_STATUS(gcl_print_memory<F16>(handle, output, "unsqueeze_output"));
#endif
    return SUCCESS;
}

EE unsqueeze_mali_fp16(
    GCLHandle_t handle, TensorDesc inputDesc, GCLMem_t input, TensorDesc outputDesc, GCLMem_t output)
{
    CHECK_STATUS(unsqueeze_checkpara_mali_fp16(inputDesc, outputDesc));
    CHECK_STATUS(fill_output_zero(handle, output, outputDesc));
    CHECK_STATUS(unsqueeze_core_mali_fp16(handle, inputDesc, input, outputDesc, output));
    return SUCCESS;
}
