#ifndef SGXFREQ_H
#define SGXFREQ_H

#include <linux/device.h>

#define SGXFREQ_NAME_LEN 16

struct sgxfreq_sgx_data {
	bool clk_on;
	bool active;
};

struct sgxfreq_governor {
	char name[SGXFREQ_NAME_LEN];
	int (*gov_start) (struct sgxfreq_sgx_data *data);
	void (*gov_stop) (void);
	void (*sgx_clk_on) (void);
	void (*sgx_clk_off) (void);
	void (*sgx_active) (void);
	void (*sgx_idle) (void);
	struct list_head governor_list;
};

/* sgxfreq_init must be called before any other api */
int sgxfreq_init(struct device *dev);
int sgxfreq_deinit(void);

int sgxfreq_register_governor(struct sgxfreq_governor *governor);
void sgxfreq_unregister_governor(struct sgxfreq_governor *governor);

int sgxfreq_set_governor(const char *name);

int sgxfreq_get_freq_list(unsigned long **pfreq_list);

unsigned long sgxfreq_get_freq_min(void);
unsigned long sgxfreq_get_freq_max(void);

unsigned long sgxfreq_get_freq_floor(unsigned long freq);
unsigned long sgxfreq_get_freq_ceil(unsigned long freq);

unsigned long sgxfreq_get_freq(void);
unsigned long sgxfreq_get_freq_request(void);
unsigned long sgxfreq_get_freq_limit(void);

unsigned long sgxfreq_set_freq_request(unsigned long freq_request);
unsigned long sgxfreq_set_freq_limit(unsigned long freq_limit);

/* External notifications to sgxfreq */
void sgxfreq_notif_sgx_clk_on(void);
void sgxfreq_notif_sgx_clk_off(void);
void sgxfreq_notif_sgx_active(void);
void sgxfreq_notif_sgx_idle(void);

#endif
