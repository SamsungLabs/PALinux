## CFI Precision

### Measure the precision (Functional evaluation)

1. Compile the kernel with PALinux
   ```
    $ rm -f linux/pal_dump.txt
    $ make clean-linux
    $ make config-all
    $ cd linux/
	$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- oldconfig  
	$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j4 Image dtbs
   ```
   after kernel build is done, you can see `linux/pal_dump.txt` that contains context information generated during compilation.

2. Check information about allowed target, which corresponds to that of Table2 in our paper
    ```
    # `/home/pal/pal_dump.txt`, created by kernel build, has to be given as input.
    python scripts/dump-to-csv.py /home/pal/pal_dump.txt > result.csv
	python scripts/csv-for-graph.py allowed-target result.csv > graph-allowedtarget.csv
	python scripts/print-percent.py graph-allowedtarget.csv
	```

   its outcome would look like
	```
    Total: 3726
    >100: 3 0.08%
    <=5: 3383 90.79%
    Max: 207
	```

3. Check information about context diversity, which corresponds to that of Table3 in our paper
    ```
    python scripts/dump-to-csv.py /data/pal_dump.txt > result.csv
	python scripts/csv-for-graph.py diversity result.csv > graph-diversity.csv
	```

    its outcome would look like
    ```
    Total: 3726
    >100: 1 0.03%
    <=5: 3536 94.9%
    Max: 110
    ```

---

### Reproducing results of Table2 and Table3 in the paper

To support reproducing precision result of our paper (Table2),
we give four prebuilt files that contain context information derived from the built kernels used in our paper.
- `pal-dump-results/pal_dump_typesig.txt`: in which only typesig applied
- `pal-dump-results/pal_dump_objtype.txt`: in which typesig+objtype applied
- `pal-dump-results/pal_dump_objtype_objbind.txt`: in which typesig+objtype+objbind applied
- `pal-dump-results/pal_dump_objtype_objbind_retbind.txt`: in which typesig+objtype+objbind+retbind applied

You can give the helper script these prebuilt files to reproduce results of Table2.
```
python scripts/dump-to-csv.py [prebuilt file you want to check] > result.csv
python scripts/csv-for-graph.py allowed-target result.csv > graph-allowedtarget.csv
python scripts/print-percent.py graph-allowedtarget.csv
```
If you do with `pal_dump_objtype_objbind_retbind.txt`, you would get the result of `+objbind+retbind` in Table 2.
Likewise, with `pal_dump_typesig.txt`, it is expected to see the result of `typesig` in Table 2.

To reproduce the result of Table3,
```
python scripts/dump-to-csv.py pal-dump-results/pal_dump_objbind_retbind.txt > result.csv
python scripts/csv-for-graph.py diversity result.csv > graph-diversity.csv
python scripts/print-percent.py graph-diversity.csv
```
NOTE: the python script would say `Max: 110` that is different from that of Table 3,
this is because we manually confirmed and exlcuded two execeptions.
