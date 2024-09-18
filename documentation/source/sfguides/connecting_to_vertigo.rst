*********************
Connecting to VerTIGo
*********************

In order to setup control or access experimental data, you will need to connect
to VerTIGo. These pages list the preferred methods to do so, in order of
preference.


Method 1: Via the Router
========================

Each VerTIGo unit comes with a preconfigured router. VerTIGo is configured to
automatically join this WiFi router network if it is reachable.


1. Turn on the WiFi router
2. Turn on VerTIGo
3. With the device that you need to interface VerTIGo with (table, phone or
   laptop), connect to the ``VerTIGo Router`` network.
4. Your device will be reachable at the address ``vertigoX.local`` where X stands
   for its unit number.


.. note::

   The steps 1. and 2. can be done in a different order, but it may take up to a
   minute or two for VerTIGo to go on the router's network. Indeed, if VerTIGo
   does not find the router network, it will starts its own :ref:`ad-hoc
   network<adhoc_wifi>`. Then it will peridically check - every minute - for the
   presence of the router network and switch to that one if found.

.. _adhoc_wifi:

Method 2: Via the Ad-Hoc WiFi Network
=====================================

If the Router network is not found, VerTIGo will automatically starts an AdHoc
(routerless) WiFi Network called ``VerTIGo X`` where X is the number of the unit.

.. important::

   While the Ad Hoc WiFi is more convenient as you do not need to use a
   dedicated router, due to poor WiFi connectivity of the Raspberry Pi 5, it
   will have a very reduced effective range.

1. Turn on VerTIGo
2. With the device that you need to interface VerTIGo with (table, phone or
   laptop), connect to the ``VerTIGo X`` network.
3. Your device will be reachable at the address `vertigoX.local` where X stands
   for its unit number.


Method 3: Via a Local Arena Network (LAN)
=========================================

Using the available Ethernet port available on the top of VerTIGo, you can
connect it to a Local Area Network (LAN). It is VerTIGo is configured to use
DHCP on this interface, so with the default setup from all Internet Service
Provider (ISP) box/router, a simple ethernet wired connection is sufficient to
connect to it.


Method 4: Via Tailscale
=======================

This method is to access to the device remotely, via internet, using a tool
called `tailscale <https://tailscale.com>`_

1. Ask the tailscale administrator to be invited in the tailnet for your VerTIGo
   devices. He should also give you the tailnet's name ( For IVP, it is
   currently ``gerbil-discus.ts.net`` )
2. Install your device into the tailnet, following `taiscale documentation
   <https://tailscale.com/kb>`_
3. VerTIGo will now be available at ``vertigoD.<tailnet name>`` instead of
   ``vertigoD.local``. For IVP it will be for example
   ``vertigo1.gerbil-discus.ts.net``


Testing the Connection
======================

Once the connection is established, you may want to first check the connection
with VerTIGo. Here are the preferred method.

* Load or reload the page `http://vertigo1.local <http://vertigo1.local>`_ (or `http://vertigo2.local <http://vertigo2.local>`_)
  in any web browser.

  .. note::

     If you are using tailscale, the addresses should be
     `http://vertigo1.gerbil-discus.ts.net
     <http://vertigo1.gerbil-discus.ts.net>`_ or
     `http://vertigo2.gerbil-discus.ts.net
     <http://vertigo2.gerbil-discus.ts.net>`_.

* If there is any error, it may be a problem with the webserver running on
  VerTIGo running on the device itself. To check if the connectivity is good,
  use the ``ping`` command.

  1. Open a terminal

     * For windows: search the taskbar by pressing the ``Window`` key once then ype ``cmd`` and enter.
     * For macOS, open the Terminal application found in ``Appications > Utilities > Terminal``.

  2. Type ``ping vertigoX.local`` or ``.gerbil-discus.ts.net`` and enter. **Remenber to replace X with the unit number**

     * If succesful, you should have something similar to:

       .. code-block::

         $ ping vertigo2.gerbil-discus.ts.net
         PING vertigo2.gerbil-discus.ts.net (100.101.242.94) 56(84) bytes of data.
         64 bytes from vertigo2.gerbil-discus.ts.net (100.101.242.94): icmp_seq=1 ttl=64 time=44.2 ms
         64 bytes from vertigo2.gerbil-discus.ts.net (100.101.242.94): icmp_seq=2 ttl=64 time=7.28 ms
         64 bytes from vertigo2.gerbil-discus.ts.net (100.101.242.94): icmp_seq=3 ttl=64 time=6.72 ms

     * If nothing shows up after a few seconds or the message ``ping:
       vertigo1.local: Name or service not known`` appears, it means that the
       connection is not established.

  3. Press ``Ctrl + C`` to stop the ping command


Troubleshooting the Connection
==============================

If you cannot establish a connection.

* Check that the router is indeed on.
* Wait for a few minutes to be sure that VerTIGo is connected to the router.
* Check that your device is on the expected WiFi network
* Try to reboot VerTIGo.
* Change your method of connection, using an ethernet cable and another router,
  or via tailscale.
