#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x37a0cba, "kfree" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x7a8006ac, "pcpu_hot" },
	{ 0xad73041f, "autoremove_wake_function" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x4a31da6e, "usb_bulk_msg" },
	{ 0xd5fd90f1, "prepare_to_wait" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0xac6425c8, "usb_register_dev" },
	{ 0xf9636917, "kmalloc_caches" },
	{ 0xcdb16d0f, "kmalloc_trace" },
	{ 0x9ed12e20, "kmalloc_large" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xa9defe90, "usb_deregister" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xe7935b03, "usb_register_driver" },
	{ 0xdb4b2ee7, "usb_deregister_dev" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x33bd423, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v0547p1095d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "E922BC80B7D3B3575061D6E");
MODULE_INFO(rhelversion, "9.7");
