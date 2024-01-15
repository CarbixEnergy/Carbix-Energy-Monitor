=========================================
Carbix Energy Monitor Project
=========================================

This is an open-source, open hardware project for monitoring energy,
forked from `IotaWatt <https://github.com/boblemaire/IoTaWatt>`_.
**We are committed to continuously optimizing supply chain costs to provide affordable hardware devices
and promote updates for this project.**

It can use any of dozens of common current transformers and will report the data locally via an integrated web server,
or upload to any of several third-party energy websites/databases.
Configuration and administration are through an easy-to-use browser-based utility running on a computer,
tablet, or smartphone.

CarbixEnergyMonitor is a full-function standalone energy monitor with the capability to store and
serve up to 15 years (or more) of comprehensive usage data and has an integrated web server with visualization apps.
At the same time, it can upload data to any of several (and counting) web-based databases and reporting systems.

Hardware is open and uses an ESP8266 nodeMCU, MCP3208 12-bit ADCs, an RTC, and an 8G SD card.

Metrics accumulated to 5-second resolution are Voltage(V), Power(Watts), Energy(kWh), VA, and PF.
While not certified to a standard, users typically report accuracy within 1% of their revenue meters.

Taking advantage of China's cost-effective supply chain, we will provide more reliable,
lower-priced energy monitors to quickly promote project development.
We will sell Carbix energy monitor kit and current sensor accessories on Amazon US soon,
allowing users to purchase according to their needs. For this equipment,
there is still a lot of room for cost optimization, which requires us to have a certain scale effect,
and we will work hard for this.

Thanks to Bob Lemaire the founder of IotaWatt for doing such a great job, we will continue to promote the open source development of this project.

`Documentation <https://carbixenergymonitor.readthedocs.io>`_

.. image:: Docs/pics/status/inputsOutputsDisplay.png
    :scale: 20 %
    :align: left

.. image:: Docs/pics/graphMultichannel.jpg
    :scale: 20 %
    :align: right

.. image:: Docs/pics/outputs/totalPowerOutput.png
    :scale: 20 %
    :align: center

.. image:: Docs/pics/PVoutput/PVoutputDisplay.png
    :scale: 20 %
    :align: center

.. image:: Docs/pics/influxDBGrafana.png
    :scale: 20 %
    :align: center