/* Implementation of remote procedure calls.
 */

#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gnutls/gnutls.h>
#include "test_gnutls_svc_gnutls.h"
#include "test_gnutls.h"

static int count = 0;		/* Count number of calls. */

char **
test_hello_1_svc (char **str, struct svc_req *req)
{
  static char *ptr;
  int len = strlen (*str);
  /* XXX What cleans up this string? */
  ptr = malloc (6 + len + 1);
  strcpy (ptr, "Hello ");
  strcpy (ptr+6, *str);
  printf ("%s\n", ptr);
  count++;
  return &ptr;
}

char **
test_ctime_1_svc (void *v, struct svc_req *req)
{
  static char *ptr;
  time_t t;
  time (&t);
  ptr = ctime (&t);
  count++;
  return &ptr;
}

void *
test_echo_1_svc (void *v, struct svc_req *req)
{
  count++;
  return &count;
}

int *
test_num_calls_1_svc (void *v, struct svc_req *req)
{
  static int c;
  c = count;
  count = 0;
  return &c;
}

/* This is autogenerated, in test_gnutls_svc.c */
extern void testprog_1 (struct svc_req *rqstp, register SVCXPRT *transp);

#define KEYFILE "newkey.pem"
#define CERTFILE "newcert.pem"
#define CAFILE "demoCA/cacert.pem"
//#define CRLFILE "crl.pem"

static gnutls_certificate_credentials_t x509_cred;
static gnutls_dh_params_t dh_params;

#define DH_BITS 1024

static int
generate_dh_params (void)
{

  /* Generate Diffie Hellman parameters - for use with DHE
   * kx algorithms. These should be discarded and regenerated
   * once a day, once a week or once a month. Depending on the
   * security requirements.
   */
  gnutls_dh_params_init (&dh_params);
  gnutls_dh_params_generate2 (dh_params, DH_BITS);

  return 0;
}

/* In this case we have to supply main() because
 * we want to provide one which listens on our chosen
 * port.  This is based on the auto-generated version
 * (see test_svc.c).
 */
int
main (int argc, char **argv)
{
  gnutls_global_init ();

  gnutls_certificate_allocate_credentials (&x509_cred);
  gnutls_certificate_set_x509_trust_file (x509_cred, CAFILE,
					  GNUTLS_X509_FMT_PEM);

  //  gnutls_certificate_set_x509_crl_file (x509_cred, CRLFILE,
  //					GNUTLS_X509_FMT_PEM);

  gnutls_certificate_set_x509_key_file (x509_cred, CERTFILE, KEYFILE,
					GNUTLS_X509_FMT_PEM);

  generate_dh_params ();

  gnutls_certificate_set_dh_params (x509_cred, dh_params);

  int sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1) { perror ("socket"); exit (1); }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (5000);
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK); // listen only on 127.0.0.1
  if (bind (sock, (struct sockaddr *) &addr, sizeof addr) == -1)
    { perror ("bind"); exit (1); }

  /* Documentation states: "If the socket is not bound to a local TCP
   * port, then this routine binds it to an arbitrary port."
   * In fact svctcp_create tries to bind the socket again, which
   * fails (because it is already bound above), and this error is
   * ignored.  The end result seems to be correct, however if
   * stracing you'll see some strange things.
   */
  SVCXPRT *transp = svcgnutls_create (sock, 0, 0, x509_cred);
  if (!transp) {
    fprintf (stderr, "cannot create GnuTLS service\n");
    exit (1);
  }

  /* Because final arg is 0, this will not register with portmap. */
  if (!svc_register (transp, TESTPROG, TESTPROG_VERS1, testprog_1, 0)) {
    fprintf (stderr, "unable to register (TESTPROG, TESTPROG_VERS1, 0)\n");
    exit (1);
  }

  svc_run ();
  fprintf (stderr, "svc_run returned\n");
  exit (1);
  /* NOTREACHED */
}