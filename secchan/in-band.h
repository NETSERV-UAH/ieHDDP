/* Copyright (c) 2008 The Board of Trustees of The Leland Stanford
 * Junior University
 *
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */

#ifndef IN_BAND_H
#define IN_BAND_H 1
/*Modificaciones Boby UAH*/
#include "stdint.h"

/*+++FIN+++*/

struct port_watcher;
struct rconn;
struct secchan;
struct settings;
struct switch_status;
struct in_band_data;
struct in_addr;

void in_band_start(struct secchan *, const struct settings *,
                   struct switch_status *, struct port_watcher *,
                   struct rconn *remote);

//Modificaciones Boby UAH//

void install_in_band_rules_UAH(struct rconn *local_rconn, struct in_band_data *in_band);
void install_new_localport_rules_UAH(struct rconn *local_rconn, uint32_t *new_local_port, struct in_addr *local_ip, struct in_addr *controller_ip, uint8_t *mac, uint32_t *old_local_port);

//++++++//

#endif /* in-band.h */
