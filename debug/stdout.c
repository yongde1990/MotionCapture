
#include <config.h>
#include <debug_manager.h>
#include <stdio.h>
#include <string.h>


/**********************************************************************
 * �������ƣ� StdoutDebugPrint
 * ���������� "��׼�������ͨ��"���������
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� �����Ϣ�ĳ���
 ***********************************************************************/
static int StdoutDebugPrint(char *strData)
{
	/* ֱ�Ӱ������Ϣ��printf��ӡ���� */
	printf("%s", strData);
	return strlen(strData);	
}

static T_DebugOpr g_tStdoutDbgOpr = {
	.name       = "stdout",
	.isCanUse   = 1,                 /* 1��ʾ��ʹ���������������Ϣ */
	.DebugPrint = StdoutDebugPrint,  /* ��ӡ���� */
};

/**********************************************************************
 * �������ƣ� StdoutInit
 * ���������� ע��"��׼�������ͨ��", ��g_tStdoutDbgOpr�ṹ�����������
 * ��������� ��
 * ��������� ��
 * �� �� ֵ�� 0 - �ɹ�, ����ֵ - ʧ��
 ***********************************************************************/
int StdoutInit(void)
{
	return RegisterDebugOpr(&g_tStdoutDbgOpr);
}
