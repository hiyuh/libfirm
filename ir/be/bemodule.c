/*
 * Author:      Matthias Braun
 * Date:        29.09.2005
 * Copyright:   (c) Universitaet Karlsruhe
 * Licence:     This file protected by GPL -  GNU GENERAL PUBLIC LICENSE.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "bemodule_t.h"
#include "xmalloc.h"

void be_init_sched(void);
void be_init_blocksched(void);
void be_init_spill(void);
void be_init_listsched(void);
void be_init_schedrss(void);
void be_init_chordal(void);
void be_init_copycoal(void);
void be_init_copyheur2(void);
void be_init_raextern(void);
void be_init_copystat(void);
void be_init_arch_ia32(void);
void be_init_arch_ppc32(void);
void be_init_arch_mips(void);
void be_init_arch_arm(void);
void be_init_ilpsched(void);
void be_init_copyilp(void);
void be_init_spillremat(void);
void be_init_javacoal(void);
void be_init_ra(void);

void be_quit_copystat(void);

void be_init_modules(void)
{
	static int run_once = 0;

	if(run_once)
		return;
	run_once = 1;

	be_init_sched();
	be_init_blocksched();
	be_init_spill();
	be_init_listsched();
	be_init_schedrss();
	be_init_chordal();
	be_init_copycoal();
	be_init_copyheur2();
	be_init_raextern();
	be_init_copystat();
	be_init_ra();

	be_init_arch_ia32();
	be_init_arch_ppc32();
	be_init_arch_mips();
	be_init_arch_arm();

#ifdef WITH_ILP
	be_init_ilpsched();
	be_init_copyilp();
	be_init_spillremat();
#endif

#ifdef WITH_JVM
	be_init_javacoal();
#endif
}

void be_quit_modules(void)
{
	be_quit_copystat();
}

//---------------------------------------------------------------------------

#ifdef WITH_LIBCORE
typedef struct module_opt_data_t {
	void **var;
	be_module_list_entry_t * const *list_head;
} module_opt_data_t;

static int set_opt_module(const char *name, lc_opt_type_t type, void *data,
                          size_t length, ...)
{
	module_opt_data_t *moddata = data;
	va_list args;
	const char* opt;
	const be_module_list_entry_t *module;

	va_start(args, length);
	opt = va_arg(args, const char*);

	for(module = *(moddata->list_head); module != NULL; module = module->next) {
		if(strcmp(module->name, opt) == 0) {
			*(moddata->var) = module->data;
			break;
		}
	}
	va_end(args);

	return 0;
}

int dump_opt_module(char *buf, size_t buflen, const char *name,
                    lc_opt_type_t type, void *data, size_t length)
{
	module_opt_data_t *moddata = data;
	const be_module_list_entry_t *module;

	for(module = *(moddata->list_head); module != NULL; module = module->next) {
		if(module->data == *(moddata->var)) {
			snprintf(buf, buflen, "%s", module->name);
			return strlen(buf);
		}
	}

	snprintf(buf, buflen, "none");
	return strlen(buf);
}

int dump_opt_module_vals(char *buf, size_t buflen, const char *name,
                         lc_opt_type_t type, void *data, size_t len)
{
	module_opt_data_t *moddata = data;
	const be_module_list_entry_t *module;
	char *p = buf;

	for(module = *(moddata->list_head); module != NULL; module = module->next) {
		size_t len = strlen(module->name);

		if(module != *(moddata->list_head)) {
			p = strncat(p, ", ", buflen - 1);
			buflen -= 2;
		}

		p = strncat(p, module->name, buflen - 1);
		if(len >= buflen) {
			break;
		}
		buflen -= len;
	}

	return strlen(buf);
}


void be_add_module_to_list(be_module_list_entry_t **list_head, const char *name,
                           void *module)
{
	be_module_list_entry_t *entry = xmalloc(sizeof(entry[0]));
	entry->name = name;
	entry->data = module;
	entry->next = *list_head;
	*list_head = entry->next;
}

void be_add_module_list_opt(lc_opt_entry_t *grp, const char *name,
                            const char *description,
                            be_module_list_entry_t * const * list_head,
                            void **var)
{
	module_opt_data_t *moddata = xmalloc(sizeof(moddata[0]));
	moddata->var = var;
	moddata->list_head = list_head;

	lc_opt_add_opt(grp, name, description, lc_opt_type_enum,
	               moddata, sizeof(moddata[0]),
	               set_opt_module, dump_opt_module, dump_opt_module_vals,
				   NULL);
}

#endif
