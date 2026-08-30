function currentReleaseVersion() {
	
	// hack to fix updating for versions prior to 2.3
	var url = "about:zoolrunner";
	var upLink = document.getElementById('updateNotifier');
	upLink.innerHTML = '<a href="' + url + '">New ZoolRunner Version Available</a>';
	
	return 230;
}