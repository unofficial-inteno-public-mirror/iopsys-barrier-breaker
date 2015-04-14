L.ui.view.extend({
	title: L.tr('Internet Firewall'),

	execute: function() {
		var deferred = $.Deferred();	/* Create a new Deferred object */
		deferred.resolve();		/* Resolve it immediately, there is nothing that could fail */
		return deferred.promise();	/* Return Promise object (a subset of Deferred) */
	}

	
});
