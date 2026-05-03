/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 bmax121. All Rights Reserved.
 * Copyright (C) 2024 lzghzr. All Rights Reserved.
 * Copyright (C) 2026 polygraphene. All Rights Reserved.
 */
// Fork of https://github.com/lzghzr/APatch_kpm/blob/main/hosts_redirect/
#define filp_close filp_close_
#include <accctl.h>
#include <hook.h>
#include <kpmodule.h>
#include <kputils.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>

#include "splfix.h"
#include "hr_utils.h"
#undef filp_close

KPM_NAME("splfix-kpm");
KPM_VERSION(MYKPM_VERSION);
KPM_LICENSE("GPL v2");
KPM_AUTHOR("polygraphene");
KPM_DESCRIPTION("Modify /vendor/build.prop to fix vendor SPL.");

#define TAG "splfix-kpm: "

struct open_flags;
struct file* (*do_filp_open)(int dfd, struct filename* pathname, const struct open_flags* op);
ssize_t (*vfs_read)(struct file *, char __user *, size_t, loff_t *);
int (*filp_close)(struct file *filp, fl_owner_t id);

unsigned long kfunc_def(__arch_copy_from_user)(void *, const void __user *, unsigned long);
unsigned long kfunc_def(__arch_copy_to_user)(void __user *to, const void *from, unsigned long n);
void *kfunc_def(__kmalloc)(size_t size, gfp_t flags);
void kfunc_def(kfree)(const void *objp);

#define VENDOR_BUILD_PROP "/vendor/build.prop"

// Only one hook at a time.
struct file *hooked_file = NULL;

static void do_filp_open_before(hook_fargs3_t* args, void* udata) {
}

static void do_filp_open_after(hook_fargs3_t* args, void* udata) {
  struct file *filp = (struct file *) args->ret;
  if (IS_ERR(filp)) {
      return;
  }
  if (current_uid() != 0)
    return;

  struct filename* pathname = (struct filename*)args->arg1;

  if (unlikely(!strcmp(pathname->name, VENDOR_BUILD_PROP))) {
    hooked_file = (struct file *)args->ret;
    pr_info(TAG "Opened %s filp:%p.\n", VENDOR_BUILD_PROP, hooked_file);
  }
}

static void filp_close_before(hook_fargs2_t* args, void* udata) {
    struct file *filp = (struct file *) args->arg0;
    if (hooked_file != NULL && filp == hooked_file) {
        pr_info(TAG "Close %s.\n", VENDOR_BUILD_PROP);
        hooked_file = NULL;
    }
}
static void filp_close_after(hook_fargs2_t* args, void* udata) {
}

static void vfs_read_before(hook_fargs4_t* args, void* udata) {
}

void *memmem(const void *haystack, size_t haystack_len,
             const void *needle, size_t needle_len)
{
    const char *h = haystack;
    const char *n = needle;

    if (needle_len == 0)
        return (void *)haystack;

    if (haystack_len < needle_len)
        return NULL;

    while (haystack_len >= needle_len) {
        if (memcmp(h, n, needle_len) == 0)
            return (void *)h;
        h++;
        haystack_len--;
    }

    return NULL;
}

static void vfs_read_after(hook_fargs4_t* args, void* udata) {
    struct file *filp = (struct file *) args->arg0;
    char __user *ptr = (char __user *) args->arg1;
    ssize_t len = (ssize_t) args->arg2;
    loff_t *off = (loff_t *) args->arg3;
    if (hooked_file != NULL && filp == hooked_file) {
        pr_info(TAG "Read %s len=%ld.\n", VENDOR_BUILD_PROP, len);
        if (off) {
            pr_info(TAG "off=%ld.\n", *off);
        }
        if (len <= 0) {
            pr_info(TAG "invalid length=%ld\n", len);
            return;
        }
        
        char *buf = kf___kmalloc(len, 0);
        if (buf == NULL) {
            pr_err(TAG "Alloc failed\n");
            return;
        }
        memset(buf, 0, len);
        long ret = kf___arch_copy_from_user(buf, ptr, len);
        if (ret != 0) {
            pr_info(TAG "_copy_from_user: failed: %ld\n", ret);
            kf_kfree(buf);
            return;
        }

        const char *pattern = "ro.vendor.build.security_patch=";
        const char *replace = "2026-04-05";
        char *found = memmem(buf, len, pattern, strlen(pattern));
        if (found) {
            const char *value = found + strlen(pattern);
            int value_len = 0;
            while (value[value_len] != '\0' && value[value_len] != '\n' && value_len < len) { value_len++; }
            if (value_len != strlen(replace)) {
                pr_info(TAG "Error: Value length is not the same length as replace value.\n");
                return;
            }
            ret = kf___arch_copy_to_user(ptr + (value - buf), replace, strlen(replace));
            pr_info(TAG "Replaced: %ld\n", ret);
        }
        kf_kfree(buf);
    }
}

static long fixspl_init(const char* args, const char* event, void* __user reserved) {
  pr_info(TAG "init\n");
  kfunc_lookup_name(__arch_copy_from_user);
  kfunc_lookup_name(__arch_copy_to_user);
  kfunc_lookup_name(__kmalloc);
  kfunc_lookup_name(kfree);
  pr_info(TAG "__arch_copy_from_user:%p\n", kf___arch_copy_from_user);
  pr_info(TAG "__arch_copy_to_user:%p\n", kf___arch_copy_to_user);
  pr_info(TAG "kmalloc:%p\n", kf___kmalloc);
  pr_info(TAG "kfree:%p\n", kf_kfree);
  if (kf___arch_copy_to_user == NULL || kf___arch_copy_from_user == NULL || kf___kmalloc == NULL || kf_kfree == NULL) {
      pr_err(TAG "Failed to get func addresses.\n");
      return -ENOENT;
  }

  lookup_name(do_filp_open);
  lookup_name(filp_close);
  lookup_name(vfs_read);

  hook_func(do_filp_open, 3, do_filp_open_before, do_filp_open_after, 0);
  hook_func(filp_close, 2, filp_close_before, filp_close_after, 0);
  hook_func(vfs_read, 4, vfs_read_before, vfs_read_after, 0);
  return 0;
}

static long fixspl_exit(void* __user reserved) {
  unhook_func(do_filp_open);
  unhook_func(filp_close);
  unhook_func(vfs_read);
  return 0;
}

KPM_INIT(fixspl_init);
KPM_EXIT(fixspl_exit);
