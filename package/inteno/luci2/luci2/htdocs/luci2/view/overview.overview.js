L.ui.view.extend({

	getLanHosts: L.rpc.declare({
		object: 'router',
		method: 'clients',
		expect: { '': { } }
	}),
	
/* Top Part*/
	renderUplink: function(uplinkObj) {
		//Need Uplink Type
		var device = uplinkObj.getDevice()
		var dest = $('<div />').attr('id', 'overviewContent');
		var imgDisable = L.globals.resource + '/img/overview/overview-background-disable.png';
		if ( !device.name())
			return dest.append ($('<img />').attr('src', imgDisable));
		var devicename = device.name();
		switch (true) {
			case /eth0/.test(devicename):
				dest.addClass('eth');
				break;
			case /atm0/.test(devicename):
				dest.addClass('adsl');
				break;
			case /ptm0/.test(devicename):
				dest.addClass('vdsl');
				break;
			case /wwan0/.test(devicename):
				dest.addClass('lte');
				break;
		}
		return dest;		
	},
	
	renderCpe: function() {
		var dest = $('<div />');
		var imgCpe = L.globals.resource + '/img/overview/overview-cpe.png';
		return dest.append($('<img />').attr('src', imgCpe));
	},
		
	renderSignalChart: function(uplink)  {
		//Need of signal Quality ==> image signla quality
		//var self = this;
		var dest = $('<div />');
		var deviceName = uplink.getDevice().name();
		if ( /wwan0/.test(deviceName) == true ) {
			dest.addClass('signal-on');
		} else {
			var history = uplink.getTrafficHistory();
			var tx_rate = history.tx_bytes[history.tx_bytes.length - 1];
			var rx_rate = history.rx_bytes[history.rx_bytes.length - 1];
			dest.append($('<div />').text('↑ %.2mB'.format(tx_rate)).addClass("tR"))
				.append($('<div />').text('↓ %.2mB'.format(rx_rate)).addClass("tR"))
				.append($('<img />').attr('src', L.globals.resource + '/img/overview/overview-line-green.png'));
		}
		return dest;
	},
	
/* Bottom Part */
	renderDeviceCaption: function(captionName) {
		switch (captionName) {
			case "WIFI":
				id="captionWifi";
				break;
			case "ETHERNET":
				id="captionEthernet";
				break;
			case "USB":
				id="captionUsb";
				break;
			case "PHONE":
				id="captionPhone";
				break;
		}
		dest = $('<div />').addClass('with-border').attr('id', id);
		dest.append($('<p />').text(L.tr(captionName)));
		return dest;
	},
	
	/**Wifi Column*/
	renderAssocDev : function () {
		var self = this;
		var dest = $('<div />').attr('id', 'assocDevice').addClass('with-border');
		dest.append($('<table />'));
		self.getLanHosts().then(function(clients) {
			var i = 1 
			for (var clt in clients) {
				var client = clients[clt];
				if(client.connected == true) {
					var hostname = client.hostname
					if (hostname == "") {
						var hostname = "Device_"+i;
						i++;
					}
					dest.append($('<tr />')
							.append ($('<td />').addClass ('smartphone'))
							.append ($('<td />')
								.append ($('<p />').text(L.tr(hostname)))
								.append ($('<p />').text(L.tr(client.ipaddr)))
							)
						);
				}
			}
			dest.append($('<br />'));
			if($("#wmore").length == 0)
				dest.append($('<div />').addClass("bigBold smallArrow").attr("id","wmore").text(L.tr('More Device')));
			$('#assocDevice').find('tr:gt(2)').hide();
			$('#assocDevice').find('br:gt(2)').hide();
			$("#wmore").on("click", function(event) {
				var $link = $(this);
				//event.preventDefault();
				$('#assocDevice').find('tr:gt(2)').toggle();
				$('#assocDevice').find('br:gt(2)').toggle();
				if ($link.text() === "More Device")
					$link.text('Less Device');
				else
					$link.text('More Device');
			});
		});
		return dest;
	},
	
	renderWifiParams: function() {
		var dest = $('<div />')
				   .attr('id', 'wifi_params')
				   .addClass('with-border');
		var wps_status = 'ON';
		var schedule_status = 'ON';
		dest.append($('<table />')
					.addClass('table table-condensed table-hover')
					.append($('<tr />')
						.append ($('<td />').attr ('id','wps'))
						.append ($('<td />')
							.append($('<p />').addClass('greyItalic').text(' WPS is ' + wps_status))
						)
					)
					.append($('<tr />'))
					.append($('<tr />')
						.append ($('<td />').attr ('id','wifi-schedule'))
						.append ($('<td />')
							.append($('<p />').addClass('greyItalic').text('WIFI schedule is ' + schedule_status))
						)
					)
				);
		dest.append($('<br />'));
		return dest;
	},
	
	renderGuestWifi: function() {
		var dest = $('<div />')
				   .addClass('divGuestWifi with-border')
				   .append($('<p />')
				   .addClass ('hlGuestWifi')
				   .text('Guest Wifi'));
		return dest;
	},
	
	/**Ethernet Column*/
	renderLanHosts: function () {
		var self = this;
		dest = $('<div />').attr('id', 'lanhosttable');
		dest.append($('<table />'));
		self.getLanHosts().then(function(clients) {
			var i = 1 
			for (var clt in clients) {
				var client = clients[clt];
				if(client.connected == true) {
					var hostname = client.hostname
					if (hostname == "") {
						var hostname = "Device_"+i;
						i++;
					}
					dest.append($('<tr />')
							.append ($('<td />').addClass ('tv'))
							.append ($('<td />')
								.append ($('<p />').text(L.tr(hostname)))
								.append ($('<p />').text(L.tr(client.ipaddr)))
							)
						);
				}
			}
			dest.append($('<br />'));
			if($("#more").length == 0)
				dest.append($('<div />').addClass("bigBold smallArrow").attr("id","more").text(L.tr('More Device')));
			$('#lanhosttable').find('tr:gt(2)').hide();
			$('#lanhosttable').find('br:gt(2)').hide();
			$("#more").on("click", function(event) {
				var $link = $(this);
				//event.preventDefault();
				$('#lanhosttable').find('tr:gt(2)').toggle();
				$('#lanhosttable').find('br:gt(2)').toggle();
				if ($link.text() === "More Device")
					$link.text('Less Device');
				else
					$link.text('More Device');
			});
		});
		return dest;
	},
	/**USB column*/
	renderUsbParams: function(div) {
		var dest = div;
		var sharing_status = 'ON';
		dest.append($('<table />')
					.addClass('table table-condensed table-hover')
					.append($('<tr />')
						.append ($('<td />')
							.addClass ('arrows')
							)
						.append ($('<td />')
							.append($('<p />')
								.addClass('greyItalic')
								.text(' Sharing Status is ' + sharing_status)
							)
						)
					)
				);
		return dest;
	},
	/**Phone column*/
	renderPhoneParams: function(div) {
		var dest = div;
		var voicemailStatus = 'ON';
		var ringSchStatus = 'ON';
		var wakeUpStatus = 'ON';
		dest.append($('<table />')
					.addClass('table table-condensed table-hover')
					.append($('<tr />')
						.append ($('<td />').addClass ('answering-machine'))
						.append ($('<td />')
							.append($('<p />').addClass('greyItalic').text(' Voicemail Status is ' + voicemailStatus))
						)
					)
					.append($('<tr />')
						.append ($('<td />').addClass ('notice'))
						.append ($('<td />')
							.append($('<p />').addClass('greyItalic').text(' Ringing Schedule is ' + ringSchStatus))
						)
					)
					.append($('<tr />')
						.append ($('<td />').addClass ('wakeUpCall'))
						.append ($('<td />')
							.append($('<p />').addClass('greyItalic').text(' Wake up call is ' + wakeUpStatus))
						)
					)
				);
		return dest;
	},
/* Render Contents*/
	renderContents: function(networks) {
		var self = this;		
		for (var i = 0; i < networks.length; i++) {
			var device = networks[i].getDevice();
			if( device.name() == 'atm0.1' || device.name() == 'ptm0.1' || device.name() == 'eth0.1' || device.name() == 'wwan0') {
				uplinkObj = networks[i];
				break;
			}
		}
		var uplink = new L.ui.table({
			columns: [{
				width:   '120px',
				format: function() {
					return self.renderUplink(uplinkObj);
				}
			},{width:   '25px'},{
				width:   '120px',
				format: function() {
					return self.renderSignalChart(uplinkObj);
				}
			},{width:   '38px'},{
				width:   '120px',
				format: function () {
					return self.renderCpe();
				}
			}]		
		});
		uplink.row([ '', '', '' ]);
		uplink.insertInto('#uplinkStatus');
//-------------------------------------------------------------
		var statistics = new L.ui.table({
			columns: [{
				width:   '900px',
				format: function(){
					div=$('<div />').addClass('statistics');
					div.append($('<div />').addClass('count')
										   .attr('id','countwifi')
										   .text('0'));
					div.append($('<div />').addClass('count')
										   .attr('id','counteth')
										   .text('0'));
					div.append($('<div />').addClass('count')
										   .attr('id','countusb')
										   .text('0'));
					div.append($('<div />').addClass('count')
										   .attr('id','countphone')
										   .text('0'));
					//div.append
					return div
				}
			}]		
		});
		statistics.row([ '' ]);
		statistics.insertInto('#statistics');
		//-------------------------------------------------------------
		var network_devices = new L.ui.table({
			columns: [ {
				caption: function () {
					return self.renderDeviceCaption('WIFI');
				},
				width:   '200px',
				verticalAlign: 'top',
				format: function () {
					var content = $('<div />');
					var div1 = self.renderWifiParams();
					var div2 = self.renderAssocDev();
					var div3 = self.renderGuestWifi();
					content.append(div1)
						   .append(div2)
						   .append(div3);
					return content;
				}
			},{width:   '15px'},{
				caption: function () {
					return self.renderDeviceCaption('ETHERNET');
				},
				width:   '200px',
				verticalAlign: 'top',
				format: function() {
					return self.renderLanHosts();
				}
			},{width:   '25px'},{
				caption: function () {
					return self.renderDeviceCaption('USB');
				},
				verticalAlign: 'top',
				width:   '200px',
				format: function () {
					var div = $('<div />')
							   .addClass('with-border');
					return self.renderUsbParams(div);
				}
			},{width:   '35px'},{
				caption: function () {
					return self.renderDeviceCaption('PHONE');
				},
				verticalAlign: 'top',
				width:   '200px',
				format: function () {
					var div = $('<div />')
							   .addClass('with-border');
					return self.renderPhoneParams(div);
				}
			}]
		});
		network_devices.row([ '', '', '', '', '', '']);
		network_devices.insertInto('#map');

	},

	execute: function() {
		var self = this;
		return L.NetworkModel.init().then(function() {
			self.repeat(self.renderContents(L.NetworkModel.getInterfaces()), 3000);
		});
	}
});
