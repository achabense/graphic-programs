//2020/6/11添加了文件保存机制。
//使用了更精确的鼠标控制。
//提高了代码的可读性。

#include <graphics.h>
#include <conio.h>
#include<stdio.h>
#include<time.h>

#pragma warning(disable:26451)
#pragma warning(disable:26495)
#pragma warning(disable:4996)

///////////////////////////////////////////////////////
//图像参数。
//条件编译：仅为了美观：
///////////////////////////////////////////////////////
#define width 1024
#define height 768
#define fwidth ((double)width)
#define fheight ((double)height)
#define hdivw (fheight/fwidth)
#define ydivx hdivw
#if ((width*3==height*4)||(width==height))


///////////////////////////////////////////////////////
//复数。只实现必要的计算。
///////////////////////////////////////////////////////
struct COMPLEX
{
	double re;
	double im;
	void operator+=(COMPLEX& adder)
	{
		re += adder.re;
		im += adder.im;
	}
	void operator*=(COMPLEX& muer)
	{
		double retemp = re * muer.re - im * muer.im;
		im = im * muer.re + re * muer.im;
		re = retemp;
	}
};


///////////////////////////////////////////////////////
//mandelbrot绘图。
//使用 HSL颜色模式产生角度h1到h2的渐变色。
//这个参数模式，使得外部误差不影响内部误差：
///////////////////////////////////////////////////////
#define MAXCOLOR 64
DWORD Color[MAXCOLOR];
void InitColor()
{
	int h1 = 240, h2 = 30;
	for (int i = 0; i < MAXCOLOR / 2; i++)
	{
		Color[i] = BGR(HSLtoRGB((float)h1, 1.0f, i * 2.0f / MAXCOLOR));
		Color[MAXCOLOR - 1 - i] = BGR(HSLtoRGB((float)h2, 1.0f, i * 2.0f / MAXCOLOR));
	}
}
DWORD defaultcolor = WHITE;
void mandelbrot(double re, double im, double scopewidth, int iterations, int pow)
{
	static WCHAR buffer[256];
	DWORD* p = GetImageBuffer();
	for (int y = 0; y < height; y++)
	{
		for (int x = width - 16; x < width; x++)
			p[y * width + x] = 0x888888;
	}
	COMPLEX c, z;
	double rangex = scopewidth, rangey = rangex * fheight / fwidth;
	double fromx = re - rangex / 2, fromy = im - rangey / 2;
	for (int y = 0; y < height; y++) {
		if (y == 16) {
			swprintf(buffer, L"center re: %.7g,center im: %.7g,scope width: %.7g,power:%d,iterations:%d", re, im, scopewidth, pow, iterations);
			outtextxy(0, 0, buffer);
		}//几百次判断是不占多少资源的。
		c.im = fromy + rangey * y / fheight;
		for (int x = 0; x < width; x++) {
			c.re = fromx + rangex * x / fwidth, z = c;
			int k;
			for (k = 0; k < iterations; k++)
			{
				if (z.re * z.re + z.im * z.im > 4.0)
					break;
				COMPLEX s = z;
				for (int i = 1; i < pow; i++)
				{
					z *= s;
				}
				z += c;
			}
			p[y * width + x] = (k >= iterations) ? defaultcolor : Color[k % MAXCOLOR];
		}
	}
}
/////////////////////////////////////////////////
//封装，保证绘图期间的操作无效。
/////////////////////////////////////////////////
#define MANDELBROT()\
{\
mandelbrot(re, im, scopewidth, iterations,power);\
FlushMouseMsgBuffer(); \
while (_kbhit())char s = _getch(); \
}


///////////////////////////////////////////////////////
//窗口输入函数。
///////////////////////////////////////////////////////
bool getscopeinfo(double& re, double& im, double& scopewidth)
{
	static WCHAR buffer[130];

	double newre, newim, newwidth;
	buffer[0] = 0;
	if (InputBox(buffer, 128, L"center re:", L"inputbox", L"(nul)", 0, 0, false) == false)return false;
	if (buffer[0] == 0 || swscanf(buffer, L"%lf", &newre) == 0)return false;
	buffer[0] = 0;
	if (InputBox(buffer, 128, L"center im:", L"inputbox", L"(nul)", 0, 0, false) == false)return false;
	if (buffer[0] == 0 || swscanf(buffer, L"%lf", &newim) == 0)return false;
	buffer[0] = 0;
	if (InputBox(buffer, 128, L"scope width:", L"inputbox", L"(nul)", 0, 0, false) == false)return false;
	if (buffer[0] == 0 || swscanf(buffer, L"%lf", &newwidth) == 0)return false;

	re = newre, im = newim, scopewidth = newwidth;
	return true;
}


/////////////////////////////////////////////////
//保存函数。利用头参数检测，排除了巧合的误识别。
/////////////////////////////////////////////////
struct mandpara
{
	char headpara[32];
	double re, im, wd;
	int pw,ir;
	mandpara(double pre, double pim, double pwd, int ppw, int pir) :headpara("MandelbrotSet:12172381649164914"),re(pre), im(pim), wd(pwd), pw(ppw), ir(pir){}
	mandpara(void) {}
	bool checkokay(void)
	{
		static const char checkpara[32] = "MandelbrotSet:12172381649164914";
		for (int i = 0; i < 32; i++)
		{
			if (headpara[i] != checkpara[i])return false;
		}
		if (!(1 <= pw && pw <= 9))return false;//实际应用不存在哪怕是接近32768的iteration值。
		if (ir <= 0 || ir % 32 != 0 || ir > 32768)return false;
		return true;
	}
};
bool readfromfile(const char* fname,double& re, double& im, double& scopewidth, int& iterations, int& pow)
{
	FILE* fp = fopen(fname, "rb");
	if (fp == nullptr)return false;
	int size = ftell((fseek(fp, 0, SEEK_END), fp));
	if (size != sizeof(mandpara)) { fclose(fp); return false; }
	fseek(fp, 0, SEEK_SET);
	mandpara para;
	fread(&para, 1, sizeof(mandpara), fp);
	fclose(fp);
	if (para.checkokay() == false)return false;
	re = para.re, im = para.im, scopewidth = para.wd, iterations = para.ir, pow = para.pw;
	return true;
}
void saveasfile(double re, double im, double scopewidth, int iterations, int pow)
{
	static char svname[128]{};
	mandpara para(re, im, scopewidth, pow, iterations);
	if (para.checkokay() == false)return;
	sprintf(svname, "Mandelbrot(%x-%x).prm", (unsigned)time(NULL), (unsigned)clock());
	FILE* fp = fopen(svname, "wb");
	if (fp == nullptr)return;
	fwrite(&para, 1, sizeof(mandpara), fp);
	fclose(fp);
	putchar('\a');
}
void saveaspic(void)
{
	static WCHAR svname[128]{};
	swprintf(svname, L"Mandelbrot(%x-%x).png", (unsigned)time(NULL), (unsigned)clock());
	saveimage(svname, GetWorkingImage());
	putchar('\a');
}


/////////////////////////////////////////////////
//颜色(白)震荡子，用于封面。
/////////////////////////////////////////////////
DWORD oscillator(void)
{
	static DWORD col = 0x0;
	static int stage = 0;

	switch (stage)
	{
	case 0:
		if (col == 0xffffff)
			col = 0xfefefe, stage = 1;
		else col += 0x010101;
		break;
	case 1:
		if (col == 0x0)
			col = 0x010101, stage = 0;
		else col -= 0x010101;
		break;
	default:throw(-1);
	}
	return col;
}


int main(int ac,char** av)
{
	HWND wd = initgraph(width, height);
	SetWindowText(wd, L"Mandelbrot Set:Operations:qe,wasd,zxc,123456789,0,iop,enter,mousehit");
	RECT systrec;
	SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&systrec, 0);
	RECT thisrec;
	GetWindowRect(wd, &thisrec);
	SetWindowPos(wd, HWND_TOP, (systrec.right - thisrec.right + thisrec.left) / 2, (systrec.bottom - thisrec.bottom + thisrec.top) / 2, thisrec.right - thisrec.left, thisrec.bottom - thisrec.top, SWP_SHOWWINDOW);

	setcolor(WHITE);
	setwritemode(R2_XORPEN);//绘制模式，一次性设置。
	InitColor();

	DWORD* p = GetImageBuffer();
	for (int c = 0; c < 4 * 0xff; c++)
	{
		DWORD color = oscillator();
		for (int y = 0; y < 100; y++)
			for (int x = 0; x < 100; x++)
			{
				p[(height / 2 - 50 + y) * width + width / 2 - 50 + x] = color;
			}
		Sleep(3);
	}

	const double origre = 0, origim = 0;
	const double origXsize = 9;
	const int origiterations = 32, origpower = 2;

	double re = origre, im = origim, scopewidth = origXsize;
	int iterations = origiterations, power = origpower;

	if (ac == 1 || readfromfile(av[1], re, im, scopewidth, iterations, power) == false) {
		re = origre, im = origim, scopewidth = origXsize;
		iterations = origiterations, power = origpower;
	}

	MOUSEMSG m;
	bool isrec = false;//有无长方框。
	int recfx = 0, recfy = 0, rectx = 0, recty = 0;	//选区。

	MANDELBROT();

	while (true)
	{
		if (MouseHit()) {
			m = GetMouseMsg();
			if (m.mkLButton == false&&m.uMsg!= WM_LBUTTONUP&& isrec == true)
			{//准确的条件为：若左键放松，且不是恰好弹起，且含有方块，则删除方块。这发生在鼠标以按下的方式移出屏幕又以松开的方式回到屏幕的时候。
				isrec = false;
				rectangle(recfx, recfy, rectx, recty);
			}
			if (m.x >= 0 && m.x < width && m.y >= 0 && m.y < height)
			{
				switch (m.uMsg)
				{
				case WM_LBUTTONDOWN:
					if (isrec == false) {
						isrec = true;
						recfx = rectx = m.x;
						recfy = recty = m.y;
						rectangle(recfx, recfy, rectx, recty);
					}
					break;
				case WM_MOUSEMOVE:
					if (isrec == true)
					{
						rectangle(recfx, recfy, rectx, recty);
						rectx = m.x;
						recty = m.y;
						rectangle(recfx, recfy, rectx, recty);
					}
					break;
				case WM_LBUTTONUP:
					if (isrec == true) {
						isrec = false;
						rectangle(recfx, recfy, rectx, recty);
						rectx = m.x;
						recty = m.y;

						if (recfx == rectx || recfy == recty)break;

						if (recfx > rectx) { int tmp = recfx; recfx = rectx; rectx = tmp; }
						if (recfy > recty) { int tmp = recfy; recfy = recty; recty = tmp; }
						int recwidth = rectx - recfx;
						int recheight = recty - recfy;

						int finalwidth = (int)(recwidth > recheight / hdivw ? recwidth : recheight / hdivw);
						if (finalwidth < 4)finalwidth = 4;
						re = re + scopewidth * (recfx + rectx - width) / 2 / fwidth;
						im = im + scopewidth * (recfy + recty - height) / 2 / fwidth;//scope/fw是单格尺寸。
						scopewidth = scopewidth * finalwidth / fwidth;

						MANDELBROT();
					}
					break;
				}
			}
		}
		if (_kbhit())
		{
			char s = _getch();
			bool port = false;
			switch (s)
			{
			case '0':
				defaultcolor = defaultcolor == BLACK ? WHITE : BLACK;
				port = true;
				break;
			case '1':case '2':case'3':case '4':case '5':case '6':case '7':case '8':case '9':
				if (power != s - '0')
				{
					power = s - '0';
					port = true;
				}
				break;
			case 'w':im -= hdivw * scopewidth / 3; port = true; break;
			case 'a':re -= scopewidth / 3; port = true; break;
			case 's':im += hdivw * scopewidth / 3; port = true; break;
			case 'd':re += scopewidth / 3; port = true; break;
			case 'q':scopewidth *= 4; port = true; break;
			case 'e':scopewidth /= 4; port = true; break;
			case 'c':
				if (iterations != origiterations)
				{
					iterations = origiterations;
					port = true;
				}
				break;
			case 'x':iterations += 128; port = true; break;
			case 'z':
				if (iterations > 256)
				{
					iterations -= 128;
					port = true;
				}
				else if (iterations > 32)
				{
					iterations -= 32;
					port = true;
				}
				break;
			case 'i':
				port = getscopeinfo(re, im, scopewidth);
				if (port == false)putchar('\a');
				break;
			case 'o':
				saveasfile(re, im, scopewidth, iterations, power);
				break;
			case 'p':
				if (isrec == false)saveaspic();//禁止在有方框时保存图片。
				break;
			case 13://enter。
				re = origre, im = origim, scopewidth = origXsize, iterations = origiterations;
				port = true;
				break;
			default:break;
			}
			if (port == true)
			{
				if (isrec == true)
				{
					isrec = false;
					rectangle(recfx, recfy, rectx, recty);
				}
				MANDELBROT();
			}
		}
		Sleep(1);//防止过度占用cpu。
	}
}
#endif
