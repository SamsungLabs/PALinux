### Static Validator

**Running static validator**

1. Check prebuilt dump files in /prebuilt.
   - Download `ios.dump` and `pal-dump.zip` from release assets (`Releases/static-validator-prebuilts`)
   - iOS : ios.dump (this is a dump file for iOS kernel binary on which PA is applied)
   - PAL : pal.dump (this is a dump file built in development stage not the final stage) after decompression with `unzip pal-dump.zip`.
   - PARTS : we do not provide prebuilt file for PARTS

2. Run tool

   Run `static-validator.py` with following arguments.

   ```
   python static-validator.py <objdump file> <output file> <function identifier>
   ```

   - *objdump file* : the target kernel dump file in /prebuilt
   - *output file* : the name of output files.
   - *function identifier* : "symbol" or "pac"

   If you want to validate the iOS kernel, put "pac" into <function identifier>.

   ```
   $python static-validator.py prebuilt/ios.dump ios_report pac
   ```

   In other cases, execute the command below,

   ```
   $python static-validator.py prebuilt/pal.dump pal_report symbol
   ```

3. Check the results

   If the execution is successful, three files are generated in `out` directory. : <output file>.p1, <output file>.p2 and <output file>.p3
   

---

**How to analyze**

1. View the result

   There are three generated output files in `out` as follows.
   NOTE: They contain a lot of false alarms for now. We plan reducing the number of false alarms as future work.

   - P1. Complete protection case : `{output file}.p1` 
   - P2. No time-of-check-time-of-use case : `{output file}.p2`
   - P3. No signing oracle case : `{output file}.p3`

   Each result line is information of an unvalidated pattern.

   Result format: `[nv] func: {function name} / line: {address}: {instruction} {register}`

   	- nv : not validate
   	- *{function name}* : function name that contains the unvalidated pattern.
   	- *{address}* : start address of the unvalidated pattern.

   ```
   //result example
   [nv] func: finish_ret_to_user / line: ffff000010084288: d61f03c0 br x30
   ```

2. find unvalidated code block.

   open the input dump file and find the code block using the *{function name}* and the *{address}*.

   ```
   ffff0000100841a4 <finish_ret_to_user>:
   ...
   ffff000010084280: f294001e movk x30, #0xa000
   ffff000010084284: 911f23de add x30, x30, #0x7c8
   ffff000010084288: d61f03c0 br x30
   ...
   ```

---

**Confirmed cases (Section 4.5 - Results)**

`Section 4.5 Results` in the paper shows the number of violations confirmed by manual analysis.
We describe in more detail each violation in the following documents.
The number of confirmed cases matches with that of `Section 4.5 Results`.

- `confirmed-case/ios.md`: 5 confirmed cases that static validator found in iOS kernel binary
- `confirmed-case/parts.md`: 15 confirmed cases that static validator found in the kernel binary built by PARTS
- `confirmed-case/pal-dev.md`: 7 confirmed cases that we had faced during our development
