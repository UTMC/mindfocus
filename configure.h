/****************************
 *                          *
 * mindfocus configure file *
 *                          *
 ****************************/

/*
 *
 * ���󥹥ȡ�����������ե�����̾������
 *
 */

/* ���󥹥ȡ���ǥ��쥯�ȥ� */
#define INSTALLDIR ${HOME}/bin

/* �ǥե���ȥǡ����򥤥󥹥ȡ��뤹���� */
#define DATADIR ${HOME}/share/mindfocus

/* �ĿʹĶ���¸�ե�����(����ե�����)��̾�� */
#define DOTFILENAME ".mindfocus"


/*
 *
 * �饤�֥��λ��Ѥ�����
 *
 */


/* �͡��ʷ������¸������ĥ�����ߡ�����ʤ��ǤϺ�ư���ޤ���*/
#define SHAPE

/* XPM�ե����ޥåȤ��б�������Ȥ���������롣���ΤȤ�����¾�ɬ�ܡ� */
#define XPM
#define XPMLIBRARY -L/usr/local/X11R6/lib -lXpm


/*
 *
 * ����¾
 *
 */

/* for SunOS */
/* SunOS�ǻ��Ѥ���ݤ�������Ʋ������� */
/*#define SUNOS*/

/* ���˥᡼���������򤹤뤫�ɤ��� */
/* ������������ʤΤ�������ʤ��ȥ���ѥ��뤬�̤�ʤ��� */
#define ANIMATION

/* �������������� */
#define VERSION 0.87

/* �ǥХå��ե饰 */
/* �Ƽ�ǥХå���å�������ɽ������褦�ˤʤ� */
/*#define DEBUG*/

/*
 *
 * ������������Խ����ʤ��ǲ�������
 *
 */

#ifdef SUNOS
#define RAND_MAX 65535
#endif /* SUNOS */
