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
	{ 0x372d36bb, "module_layout" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x1ac0c403, "unregister_vsync_cb" },
	{ 0x5934392b, "fb_register_client" },
	{ 0x88a193, "__alloc_workqueue_key" },
	{ 0xd06cfa4d, "fb_pan_display" },
	{ 0xf7802486, "__aeabi_uidivmod" },
	{ 0x33543801, "queue_work" },
	{ 0x62b72b0d, "mutex_unlock" },
	{ 0x4a13d075, "PVRGetDisplayClassJTable" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xc631580a, "console_unlock" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xb50a7423, "register_vsync_cb" },
	{ 0xdc798d37, "__mutex_init" },
	{ 0x27e1a049, "printk" },
	{ 0xec6a4d04, "wait_for_completion_interruptible" },
	{ 0x328a05f1, "strncpy" },
	{ 0xfbaaf01e, "console_lock" },
	{ 0x16305289, "warn_slowpath_null" },
	{ 0xe16b893b, "mutex_lock" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0xed5e75e8, "module_put" },
	{ 0x5057614c, "registered_fb" },
	{ 0x4a0311c8, "fb_set_var" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0xcc36f32e, "fb_unregister_client" },
	{ 0x60f71cfa, "complete" },
	{ 0x18ec84, "fb_blank" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=pvrsrvkm";


MODULE_INFO(srcversion, "DDA246D5B0BE24ED1310D73");
