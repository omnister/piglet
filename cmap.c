
/* it's possible that someone can't afford color ... */
if(cmap_size < 8) {
    fprintf (stderr,"changing to black and white\n"); /* RCW */
    numpens=cmap_size-1;
    autoline=1;
    }
else {
    /* set up colors */
    int i;
    XColor exact_color, screen_color;

    char penname[8][30];

    penmap[0]=0;  strcpy(penname[0],"black");
    penmap[1]=1;  strcpy(penname[1],"white");
    penmap[2]=2;  strcpy(penname[2],"red");
    penmap[3]=4;  strcpy(penname[3],"green");
    penmap[4]=6;  strcpy(penname[4],"#7272ff");	    /* brighten up our blues */
    penmap[5]=5;  strcpy(penname[5],"cyan");
    penmap[6]=7;  strcpy(penname[6],"magenta");
    penmap[7]=3;  strcpy(penname[7],"yellow");

    /* cmap = DefaultColormap (Xdisplay, screen); /* Fails on 725 */
    if ((window=xwid(window_fd,windowname)) > 0) {
	XGetWindowAttributes (Xdisplay, window, &window_attr);
	cmap=window_attr.colormap;
    } else {
    	cmap = DefaultColormap (Xdisplay, screen); /* Fails on 725 */
	warning ("xwid() fails; using DefaultColormap()\n");
    } 
    
    for (i=0; i<=7; i++) 
	if (XAllocNamedColor(Xdisplay, cmap, penname[i], 
				&screen_color, &exact_color)!=0)
	    penmap[i] = exact_color.pixel;
	else
	    fprintf (stderr,"autoplot warning: cannot allocate color '%s'\n", 
	    penname[i]);
    }
