# run piglet in the GNU debugger to catch the wiley coyote

stty sane

echo "run"    > pig.tmp.$$
echo "bt"    >> pig.tmp.$$
echo "echo \\n" >> pig.tmp.$$
echo "echo \\n" >> pig.tmp.$$
echo "echo Piglet has crashed.  Please send a description of what you were doing \\n " >> pig.tmp.$$
echo "echo plus the last 100 lines of the file named \\\"pigtrace\\\" to \\n " >> pig.tmp.$$
echo "echo <piglet_walker@omnisterra.com>.  Thanks! - Rick Walker\\n" >> pig.tmp.$$
echo "echo \\n" >> pig.tmp.$$
echo "echo \\n" >> pig.tmp.$$
echo "quit"  >> pig.tmp.$$
echo "yes"   >> pig.tmp.$$
echo "quit"  >> pig.tmp.$$
echo "yes"   >> pig.tmp.$$

gdb --batch  -q -command pig.tmp.$$ pig.bin | tee pigtrace

cat pigtrace  | col -b > pig.trace.$$
mv pig.trace.$$ pigtrace

rm pig.tmp.$$
