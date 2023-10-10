<?xml version="1.0" encoding="UTF-8"?>
<app:application 
  xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0" 
  type="com.semtech.av.ztp" 
  name="ztp" 
  revision="bae1d86d529be1f7a2bed8f3177a72e9">
    <application-manager use="LWM2M_SW"/>
    <capabilities>
        <data>
            <encoding type="LWM2M">
                <asset default-label="ztp" id="ztp">
                        <setting path="/ztp/endpoint" type="string" default-label="/ztp/endpoint"/>
                        <setting path="/ztp/cert1" type="string" default-label="/ztp/cert1"/>
                        <setting path="/ztp/cert2" type="string" default-label="/ztp/cert2"/>
                        <setting path="/ztp/cert3" type="string" default-label="/ztp/cert3"/>
                        <setting path="/ztp/cert4" type="string" default-label="/ztp/cert4"/>
                        <setting path="/ztp/cert5" type="string" default-label="/ztp/cert5"/>
                        <setting path="/ztp/cert6" type="string" default-label="/ztp/cert6"/>
                </asset>
            </encoding>
        </data>
    </capabilities>
  <binaries>
    <binary file="ztp.wp77xx.update"/>
  </binaries>
</app:application>