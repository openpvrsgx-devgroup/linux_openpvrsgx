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
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xff178f6, "__aeabi_idivmod" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0xcfad398d, "device_destroy" },
	{ 0x46ea0122, "__register_chrdev" },
	{ 0x62b72b0d, "mutex_unlock" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x27e1a049, "printk" },
	{ 0x328a05f1, "strncpy" },
	{ 0xd69d5047, "dma_free_coherent" },
	{ 0xe16b893b, "mutex_lock" },
	{ 0xdc1e1b7d, "device_create" },
	{ 0xa2ef561a, "dma_alloc_coherent" },
	{ 0xe328af64, "PVRGetBufferClassJTable" },
	{ 0x37a0cba, "kfree" },
	{ 0xf6012f5a, "remap_pfn_range" },
	{ 0x9d669763, "memcpy" },
	{ 0xc7ef0ce2, "class_destroy" },
	{ 0x4c1d0cd4, "__class_create" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=pvrsrvkm";


MODULE_INFO(srcversion, "34D71BE8F97CAF02B6B0A3B");
