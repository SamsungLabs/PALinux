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

MODULE_LICENSE("GPL");

extern char name[];
extern int vulnerable(void);

static int pac_example_proc_show(struct seq_file *m, void *v) {
	int ret = 0;
	ret = vulnerable();

	if (ret == 0) {
		printk("Detecting %s : [Failed]\n", name);
	} else {
		printk("Detecting %s : [Succeeded]\n", name);
	}

	return 0;
}

static int pac_example_proc_open(struct inode *inode, struct file *file) {
	return single_open(file, pac_example_proc_show, NULL);
}

static const struct file_operations pac_example_proc_fops = {
	.owner = THIS_MODULE,
	.open = pac_example_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int pac_example_init(void)
{
	printk("pac example module init\n");

	proc_create("pac-example", 0, NULL, &pac_example_proc_fops);

	return 0;
}

void pac_example_cleanup(void)
{
	printk("pac example module cleanup\n");

	remove_proc_entry("pac-example", NULL);

	return;
}

module_init(pac_example_init);
module_exit(pac_example_cleanup);
