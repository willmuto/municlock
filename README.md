# municlock

This is code for driving a Muni prediction clock. The hardware is a Adafruit Huzzah ESP32 Feather with a 14-segment FeatherWing display.

https://learn.adafruit.com/adafruit-huzzah32-esp32-feather
https://learn.adafruit.com/14-segment-alpha-numeric-led-featherwing/overview

## Accessing Muni prediction data

This code uses the public nextbus HTTP REST API for retrieving prediction times. To get an estimated arrival time, you first need a stop number. You can get a stop number by querying a route with the following REST call:

http://webservices.nextbus.com/service/publicXMLFeed?command=routeConfig&a=sf-muni&r=<ROUTE#>

For the 48, the response looks something like this:

```<body copyright="All data copyright San Francisco Muni 2019.">
  <route tag="48" title="48-Quintara-24th Street" color="cc6600" oppositeColor="000000" latMin="37.73946" latMax="37.7604699" lonMin="-122.50755" lonMax="-122.3880899">
    <stop tag="3411" title="20th St & 3rd St" lat="37.7604699" lon="-122.38841" stopId="13411"/>
    <stop tag="3432" title="22nd St & 3rd St" lat="37.75801" lon="-122.3880899" stopId="13432"/>
    <stop tag="3436" title="22nd St & Minnesota St" lat="37.7578699" lon="-122.39008" stopId="13436"/>
    <stop tag="3435" title="22nd St & Iowa St" lat="37.7577499" lon="-122.39227" stopId="13435"/>
    <stop tag="7523" title="Pennsylvania Ave & 23 St" lat="37.75516" lon="-122.39301" stopId="17523"/>
    <stop tag="7524" title="Pennsylvania Ave & 25 St" lat="37.7526199" lon="-122.39276" stopId="17524"/>
    <stop tag="3507" title="25th St & Dakota St" lat="37.7524999" lon="-122.39483" stopId="13507"/>
    <stop tag="3504" title="25th St & Connecticut St" lat="37.75236" lon="-122.3963399" stopId="13504"/>
    <stop tag="3512" title="25th St & Wisconsin St" lat="37.75224" lon="-122.3983099" stopId="13512"/>
    <stop tag="3524" title="26th St & Wisconsin St" lat="37.7511099" lon="-122.3986599" stopId="13524"/>
```

The stop number is specified by `tag=####`. Once you have that identifier, you can pass it to the prediction call:

http://webservices.nextbus.com/service/publicXMLFeed?command=predictions&a=sf-muni&r=<ROUTE#>&s=<STOP#>

```
<body copyright="All data copyright San Francisco Muni 2019.">
  <predictions agencyTitle="San Francisco Muni" routeTitle="48-Quintara-24th Street" routeTag="48" stopTitle="24th St & Guerrero St" stopTag="3471">
    <direction title="Outbound to West Portal Station">
      <prediction epochTime="1551080460231" seconds="568" minutes="9" isDeparture="false" dirTag="48___O_F00" vehicle="8831" block="4891" tripTag="8250914"/>
      <prediction epochTime="1551081565747" seconds="1673" minutes="27" isDeparture="false" affectedByLayover="true" dirTag="48___O_F00" vehicle="8845" block="9591" tripTag="8250913"/>
    </direction>
    <message text="Now thru March: the K Line will operate btwn Balboa Park and Embarcadero" priority="Low"/>
    <message text="BART wkdy svc now begins at 5am. In SF use Muni 714 BART shuttle. Visit bart.gov for details" priority="Low"/>
    <message text="No Elevator at Castro Outbound" priority="Normal"/>
    <message text="Now thru March: Bus shuttles will provide T Line svc btwn Embar. and Bayshore terminal" priority="Low"/>
  </predictions>
</body>
```

## Displaying

The stops are specified in a multidimensional array, in the following order:

[ROUTE, STOP NUMBER, DIRECTION]

The direction is specified so a dot can be used to differentiate on the display.
