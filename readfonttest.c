#include "readfont.h"
#include "db.h"
#include <stdio.h>

main()
{
    XFORM *xp;

    xp = (XFORM *) malloc(sizeof(XFORM)); /* create a unit xform matrix */
    xp->r11 = 1.0;
    xp->r12 = 0.0;
    xp->r21 = 0.0;
    xp->r22 = 1.0;
    xp->dx  = 0.0;
    xp->dy  = 0.0;


    loadfont("NOTEDATA.F");
    printf("back\nisotropic\nnogrid\n");

/*    writestring("Now is the time",xp);  xp->dy--;
    writestring("for all good men",xp); xp->dy--;
    writestring("to come to the",xp);   xp->dy--;
    writestring("ABCDEFGHIJKLMNOP",xp); xp->dy--;
    writestring("QRSTUVWXYZabcdef",xp); xp->dy--;
    writestring("ghijklmnopqrstuv",xp); xp->dy--;
    writestring("wxyz0123456789~!",xp); xp->dy--;
    writestring("#$%^&*()_+=-';:",xp);  xp->dy--;
    writestring("][}{.,></?|",xp);      xp->dy--;
*/

/*
ww	|      |       |
yy	|PAN   |MINGRD |
ww	|      |       |
yyyyyy	|ADD |R|P|L|N|T|
aagga	|SHO#A;|A|+|-|I|
aayggg	|SHO |#|^|1|2|3|
yyyggg	|WIN |;|!|4|5|6|
yyyggg	|:F|:V|:Z|7|8|9|
yyyggg	|:D|:S|:V|,|0|.|
yayyp	|:W|:M|:N|:O|:R|
yaapp	|  |X|Y|+90|-90|
ww	|      |       |
aaa	|WRA |SMA |GRO |
aaa	|MOV |COP |STR |
ypy	|IDE |DEC |DIS |
ayyy	|SEA |:X|:C|500|
ww	|      |       |
rrr	|UND |POI |DEL |
yyg	|PLO | :C | :X |
yyr	|SAV |EDI |EXI |
*/

	writestring("|      |       |",xp); xp->dy--;
	writestring("|PAN   |MINGRD |",xp); xp->dy--;
	writestring("|      |       |",xp); xp->dy--;
	writestring("|ADD |R|P|L|N|T|",xp); xp->dy--;
	writestring("|SHO#A;|A|+|-|I|",xp); xp->dy--;
	writestring("|SHO |#|^|1|2|3|",xp); xp->dy--;
	writestring("|WIN |;|!|4|5|6|",xp); xp->dy--;
	writestring("|:F|:V|:Z|7|8|9|",xp); xp->dy--;
	writestring("|:D|:S|:V|,|0|.|",xp); xp->dy--;
	writestring("|:W|:M|:N|:O|:R|",xp); xp->dy--;
	writestring("|  |X|Y|+90|-90|",xp); xp->dy--;
	writestring("|      |       |",xp); xp->dy--;
	writestring("|WRA |SMA |GRO |",xp); xp->dy--;
	writestring("|MOV |COP |STR |",xp); xp->dy--;
	writestring("|IDE |DEC |DIS |",xp); xp->dy--;
	writestring("|SEA |:X|:C|500|",xp); xp->dy--;
	writestring("|      |       |",xp); xp->dy--;
	writestring("|UND |POI |DEL |",xp); xp->dy--;
	writestring("|PLO | :C | :X |",xp); xp->dy--;
	writestring("|SAV |EDI |EXI |",xp); xp->dy--;

    writestring("|;       | A|       |0|",xp); xp->dy--;
    writestring("|ADD     | R|       |1|",xp); xp->dy--;
    writestring("|MOVe    | P|       |2|",xp); xp->dy--;
    writestring("|COPy    | L| Wid=  |3|",xp); xp->dy--;
    writestring("|DELete  | C| Res=  |4|",xp); xp->dy--;
    writestring("|STRetch | N| Rot=  |5|",xp); xp->dy--;
    writestring("|GROup   | T| Font= |6|",xp); xp->dy--;
    writestring("|WRAp    | I| Xscl= |7|",xp); xp->dy--;
    writestring("|SMAsh   | ;| Mir=X |8|",xp); xp->dy--;
    writestring("|STEp S= | ,| Mir=Y |9|",xp); xp->dy--;
    writestring("|UNDo    | +|      |11|",xp); xp->dy--;
    writestring("|SHOw    | #| #A;  |12|",xp); xp->dy--;
    writestring("|IDEntify| .| ;    |13|",xp); xp->dy--;
    writestring("|TRAce   | @| ;    |14|",xp); xp->dy--;
    writestring("|WINdow  |:F| Vprt=|15|",xp); xp->dy--;
    writestring("|WIN V=2 |:N| Nest=|16|",xp); xp->dy--;
    writestring("|GRId    |:O| Zoom=|17|",xp); xp->dy--;
    writestring("|LOCk    |:P| Xabs=|18|",xp); xp->dy--;
    writestring("|DUMp    | %| Detl=|19|",xp); xp->dy--;
    writestring("|POInt   | \\| Symb=|20|",xp); xp->dy--;
    writestring("|DIStance|  | Boun=|21|",xp); xp->dy--;
    writestring("|AREa    |  | Intc=|22|",xp); xp->dy--;
    writestring("|EDIt    |  |HELp  |23|",xp); xp->dy--;
    writestring("|SAVe    | ;|EXIt; |24|",xp); xp->dy--;
}

void draw(x,y) 
double x,y;
{
    printf("%g %g\n",x,y);
}

void jump() 
{
    printf("jump\n");
}
