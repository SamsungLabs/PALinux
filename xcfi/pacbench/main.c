/*
 * Copyright (C) 2019 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Author: Sungbae Yoo <sungbae.yoo@samsung.com>
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3, or (at your option) any later
 * version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License at
 * <http://www.gnu.org/licenses/>.
 */

#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "pacbench.h"

MODULE_LICENSE("GPL");

DECLARE_COMPLETION(bm_comp);

int do_benchmark_task(void *data)
{
	struct seq_file *m = (struct seq_file*)data;
	struct feature feature;

	seq_printf(m, "----------------------------------------\n");

	check_feature(&feature, m);

	if (!feature.address.arch && !feature.address.imp_def) {
		seq_printf(m, "This CPU doesn't support PAC\n");
		seq_printf(m, "----------------------------------------\n");
		goto fallback;
	}

	evaluate_instruction(&feature, m);
	evaluate_keychange(&feature, m);

	seq_printf(m, "\n");

fallback:
	complete(&bm_comp);

	preempt_enable();

	return 0;
}

static int kpac_proc_show(struct seq_file *m, void *v) {
	struct task_struct *bench_task;
	unsigned long i;

	for_each_online_cpu(i) {
		init_completion(&bm_comp);
		bench_task = kthread_create(do_benchmark_task, (void*)m, "pac-info");
		kthread_bind(bench_task, i);
		wake_up_process(bench_task);
		wait_for_completion(&bm_comp);
	}
	return 0;
}

static int kpac_proc_open(struct inode *inode, struct file *file) {
	return single_open(file, kpac_proc_show, NULL);
}

static const struct file_operations kpac_proc_fops = {
	.owner = THIS_MODULE,
	.open = kpac_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int kpacbench_init(void)
{
	printk("kpacbench module init\n");

	proc_create("pac-bm", 0, NULL, &kpac_proc_fops);

	return 0;
}

void kpacbench_cleanup(void)
{
	printk("kpacbench module cleanup\n");

	remove_proc_entry("pac-bm", NULL);

	return;
}

module_init(kpacbench_init);
module_exit(kpacbench_cleanup);
