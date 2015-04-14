$('#indicator').css('left', '224px');
function showNHide(e1, e2) {
	$(e1).hide();
	$(e2).show();
}

var x = 0;
function showIndicator () {
	$('.whiteLayer').css('display', 'block');
	$('#indicator').css('display', 'block');
	$('#indicator').delay(940).animate({ left: '208px' }, 200);
}

function showMenuAnimated (linkElement, cat) {
	var list = $('.navigation-' + cat);
	list.css('display', '');
	var wT = 0;
	var l = $('.navigation-' + cat + ' > li').length;
	$('.navigation-' + cat + ' li').each(function () {
		wT += 100;
		var element = this;
		window.setTimeout(function () { doAnimation(element, l, linkElement); }, wT);
	});
}

function doAnimation (element, l, linkElement) {
	$(element).animate({ marginLeft: '0px', opacity: '1' }, 600, 'easeInOutQuart', function () {
		x++;
		if (x == l) {
			var link = linkElement.attr('href');
			window.location = link;
		}
	});
}

$(function () {
	$('#normalWifiClosed').live('click', function () {
		  $('#normalWifiList').animate({ height: 'show' });
		  showNHide("#normalWifiClosed", "#normalWifiExpanded");
	});
	$('#normalWifiExpanded, #normalWifiList').live('click', function () {
		  $('#normalWifiList').animate({ height: 'hide' });
		  showNHide("#normalWifiExpanded", "#normalWifiClosed");
	});

	$('#guestWifiClosed').live('click', function () {
		  $('#guestWifiList').animate({ height: 'show' });
		  showNHide("#guestWifiClosed", "#guestWifiExpanded");
	});
	$('#guestWifiExpanded, #guestWifiList').live('click', function () {
		  $('#guestWifiList').animate({ height: 'hide' });
		  showNHide("#guestWifiExpanded", "#guestWifiClosed");
	});

	$('#header nav a').click(function () {
		var ul = $('nav > ul');
		var element = $(this);

		$('#overviewContent').fadeOut();

		$('#header .active').removeClass('active');
		$(this).addClass('active');

		var cat = $(this).parent('li').attr('id');
		$('article, #langAndFirmware').animate({ width: '734px' }, 800, 'easeInOutQuint', function () {
			$('#navigation').show();
			$('#navigation li').css('opacity', '0');
		});
		showIndicator();
		$('#inner-wrap article#overview img').delay(100).fadeOut(300);

		var ul = $('nav > ul');
		ul.removeClass();
		if (messagesCat) {
			ul.addClass('clearfix seven ' + cat + 'Menu');
		} else {
			ul.addClass('clearfix ' + cat + 'Menu');
		}

		window.setTimeout(function () { showMenuAnimated(element, cat); }, 250);
		return false;
	});
});
