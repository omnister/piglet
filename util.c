
int microns2units(s)
char *s;
{
	int result;
	double in;
	extern double floor();

	sscanf(s,"%lf",&in);
	result = (int)floor(in+.5);
	return(result);
}


int sprint_fix(s,n)
char *s;
int n;
{
	int temp,
	    len;

	temp = sprintf(s,"%d",n);
	len = strlen(s);
	switch (len)
	{
		case 0:	strcpy(s,"0.00");
			break;
		case 1:	s[3] = s[0];
			s[0] = '0';
			s[1] = '.';
			s[2] = '0';
			s[4] = 0;
			break;
		case 2: s[2] = s[0];
			s[3] = s[1];
			s[0] = '0';
			s[1] = '.';
			s[4] = 0;
			break;
		default: s[len+1] = 0;
			s[len]   = s[len-1];
			s[len-1] = s[len-2];
			s[len-2] = '.';
			break;
	}
	return(temp);
}
