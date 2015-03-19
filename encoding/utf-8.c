#include <config.h>
#include <encoding_manager.h>
#include <string.h>

static int isUtf8Coding(unsigned char *pucBufHead);
static int Utf8GetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode);

static T_EncodingOpr g_tUtf8EncodingOpr = {
	.name          = "utf-8",
	.iHeadLen	   = 3,
	.isSupport     = isUtf8Coding,
	.GetCodeFrmBuf = Utf8GetCodeFrmBuf,
};

/**********************************************************************
 * 函数名称： isUtf8Coding
 * 功能描述： 判断缓冲区中数据的编码方式是否为UTF8
 * 输入参数： pucBufHead - 缓冲区首地址,一般是文本文件经过mmap得到的缓冲区地址
 * 输出参数： 无
 * 返 回 值： 0 - 不是
 *            1 - 是
 ***********************************************************************/
static int isUtf8Coding(unsigned char *pucBufHead)
{
	const char aStrUtf8[]    = {0xEF, 0xBB, 0xBF, 0};
	if (strncmp((const char*)pucBufHead, aStrUtf8, 3) == 0)
	{
		/* UTF-8 */
		return 1;
	}
	else
	{
		return 0;
	}
}

/**********************************************************************
 * 函数名称： GetPreOneBits
 * 功能描述： 获得"从bit7开始为1的位的个数"
 *            比如二进制数 11001111 有2位
 *                         11100001 有3位
 *                         01100001 有0位
 * 输入参数： ucVal - 要分析的数
 * 输出参数： 无
 * 返 回 值： "从bit7开始为1的位的个数"
 ***********************************************************************/
static int GetPreOneBits(unsigned char ucVal)
{
	int i;
	int j = 0;
	
	for (i = 7; i >= 0; i--)
	{
		if (!(ucVal & (1<<i)))
			break;
		else
			j++;
	}
	return j;

}

/**********************************************************************
 * 函数名称： Utf8GetCodeFrmBuf
 * 功能描述： 从缓冲区中取出第1个字符的UTF8编码并且转换为UNICODE值
 * 输入参数： pucBufStart - 缓冲区起始地址
 *            pucBufEnd   - 缓冲区结束地址(这个位置的字符不处理)
 * 输出参数： pdwCode     - 所取出的编码值存在这里
 * 返 回 值： 0      - 缓冲区处理完毕,没有得到编码值
 *            其他值 - 从pucBufStart开始用到了多少个字符才取得这个编码值
 ***********************************************************************/
static int Utf8GetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode)
{
	int i;	
	int iNum;
	unsigned char ucVal;
	unsigned int dwSum = 0;

	if (pucBufStart >= pucBufEnd)
	{
		/* 文件结束 */
		return 0;
	}

	ucVal = pucBufStart[0];
	iNum  = GetPreOneBits(pucBufStart[0]);

	if ((pucBufStart + iNum) > pucBufEnd)
	{
		/* 文件结束 */
		return 0;
	}

	if (iNum == 0)
	{
		/* ASCII */
		*pdwCode = pucBufStart[0];
		return 1;
	}
	else
	{
		ucVal = ucVal << iNum;
		ucVal = ucVal >> iNum;
		dwSum += ucVal;
		for (i = 1; i < iNum; i++)
		{
			ucVal = pucBufStart[i] & 0x3f;
			dwSum = dwSum << 6;
			dwSum += ucVal;			
		}
		*pdwCode = dwSum;
		return iNum;
	}
}

/**********************************************************************
 * 函数名称： Utf8EncodingInit
 * 功能描述： 根据字符的UNICODE编码值,有freetype/ascii这2种方法获得它的字符位图
 *            Utf8EncodingInit先给g_tUtf8EncodingOpr添加freetype/ascii这2种PT_FontOpr结构体;
 *            最后注册g_tUtf8EncodingOpr
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 0      - 成功
 *            其他值 - 失败
 ***********************************************************************/
int  Utf8EncodingInit(void)
{
	AddFontOprForEncoding(&g_tUtf8EncodingOpr, GetFontOpr("freetype"));
	return RegisterEncodingOpr(&g_tUtf8EncodingOpr);
}

