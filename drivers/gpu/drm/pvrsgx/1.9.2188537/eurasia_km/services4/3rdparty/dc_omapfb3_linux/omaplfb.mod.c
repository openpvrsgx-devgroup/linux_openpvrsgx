#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x88d2b212, "module_layout" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x5cbf3a05, "mutex_destroy" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x5934392b, "fb_register_client" },
	{ 0x9a82d814, "__alloc_workqueue_key" },
	{ 0xcc69e580, "fb_pan_display" },
	{ 0xf7802486, "__aeabi_uidivmod" },
	{ 0x9248094b, "queue_work" },
	{ 0xcd281907, "mutex_unlock" },
	{ 0x4a13d075, "PVRGetDisplayClassJTable" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x6dbcb1ec, "__mutex_init" },
	{ 0xea147363, "printk" },
	{ 0x94d32a88, "__tracepoint_module_get" },
	{ 0x328a05f1, "strncpy" },
	{ 0x16305289, "warn_slowpath_null" },
	{ 0xc291205c, "destroy_workqueue" },
	{ 0x86b25de9, "module_put" },
	{ 0x434fa55c, "release_console_sem" },
	{ 0xf174ed48, "acquire_console_sem" },
	{ 0x1c1597ee, "registered_fb" },
	{ 0xc27487dd, "__bug" },
	{ 0x813564a4, "fb_set_var" },
	{ 0x8a1695cc, "mutex_lock_nested" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x16f9b492, "lockdep_init_map" },
	{ 0xcc36f32e, "fb_unregister_client" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x3f0a7365, "fb_blank" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=pvrsrvkm";


MODULE_INFO(srcversion, "F6599CC74E734C065890C7C");
