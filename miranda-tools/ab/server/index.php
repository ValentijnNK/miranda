<?php
/* checkout.when contains the date/time/timezone of the build generated by gen.py, etc
and ChangeLog.part contains the delta changelog
*/
/* SF doesn't have file_get_contents() */
$checkout = file('checkout.when');
if ( $checkout === FALSE) die;
// replace any whitespace with a single space from the array
$checkout = preg_replace('/\s+/', ' ', $checkout[0]);
// create an array of words from the line, 0 = date, 1 = time, 2 = timezone
$checkout = explode(' ', $checkout);
// store YYYY-MM-DD as its sum parts
$co_date_t = explode('-', $checkout[0]);
// strip YYYY-MM-DD down to YYYYMMDD
$co_date = str_replace( '-', '', $checkout[0] );
// process time HH:MM:SS, tokenise by :
$co_time_t = explode(':', $checkout[1] );
// store HHMM
$co_time = $co_time_t[0] . $co_time_t[1];
// build the final string as YYYYMMDDHHMM
$buildstamp = $co_date . $co_time;
// the timezone
$timezone = trim ( $checkout[2] );
// build a unix timestamp, hour, min, sec , month, day, year, year
$timestamp = mktime( $co_time_t[0], $co_time_t[1], $co_time_t[2], $co_date_t[1], $co_date_t[2], $co_date_t[0] );

$FILELIST = array(
	array('miranda32', 0, 'Main application', 'strong' => 1),
	array('clist', 1, 'Classic contact list', 'strong' => 0),
	array('mwclist', 1, 'multiwin clist by Bethoven', 'strong' => 0),
	array('icq', 1, 'access ICQ', 'strong' => 0),
	array('aim', 1, 'access AIM', 'strong' => 0),
	array('jabber', 1, 'access Jabber', 'strong' => 0),
	array('msn', 1, 'access MSN', 'strong' => 0),
	array('yahoo', 1, 'access Yahoo', 'strong' => 0),
	//array('tabsrmm', 1, 'Tab/container messaging interface, SRMM based by Nightwish', 'strong' => 0),
	array('srmm', 1, 'Provides a split/single messaging window interface', 'strong' => 0)
	//array('import', 1, 'Import contacts, settings, history from other Miranda profiles, ICQ databases', 'strong' => 0),
	//array('changeinfo', 1, 'Change your ICQ user details from within Miranda', 'strong' => 0)
);

function get_file_list($mode)
{
	global $FILELIST, $buildstamp;
	$output=null;
	foreach ($FILELIST as $file) {
		$zip = $file[0] . '_' . ( $mode == 'debug' ? 'debug_' : '') . $buildstamp . '.zip';
		$size = @ filesize($zip);
		$output .= sprintf('<tr> <td>%s%s%s</td> <td> <a href="%s">%s</a> </td> <td>%d KB</td> <td>%s</td> </tr>', 
			$file['strong'] == 1 ? '<strong>' : '',
			$file[0] . ($file[1] ? '.dll':'.exe'),
			$file['strong'] == 1 ? '</strong>' : '',
			$zip, $zip, 
			$size ? $size / 1024 : 0,
			$file[2]
			);
	}
	return $output;
}
	$LFN=null;
	$CHANGES=null;
	$THIS_CHANGE=array();
	function startElement($xml, $name, $attr) {
		global $LFN, $THIS_CHANGE;
		$LFN = $name;
	}
	function endElement($xml, $name) {
		global $LFN, $THIS_CHANGE, $CHANGES;
		if ( $name == 'ENTRY') 
		{
			$CHANGES[]=$THIS_CHANGE;
			$THIS_CHANGE=array();
		}
		//echo sprintf(" \n EOF %s, %s  \n", $LFN, $name);
	}
	function cdataElement($xml, $data) {
		global $LFN, $THIS_CHANGE;				
		if ( $LFN == 'MSG' ) {		
			$THIS_CHANGE[$LFN] .= $data;
		} else {
			$THIS_CHANGE[$LFN][] = $data;
		}
		//echo sprintf("name = %s, data = %s", $LFN, $data);
	}
	$xml = xml_parser_create();
	if ( $xml ) {		
		$cl = implode('', @file('ChangeLog_'.$buildstamp.'.part'));
		xml_set_element_handler($xml, 'startElement', 'endElement');
		xml_set_character_data_handler($xml, 'cdataElement');
		if ( xml_parse($xml, $cl, true) ) {
			// good
		}
		xml_parser_free($xml);
	}

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
	<head>
		<title>Miranda IM: Alpha Builds, CVS snapshots </title>
		<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />
		<meta http-equiv="cache-control" content="no-cache" />
		<meta http-equiv="pragma" content="no-cache" />
		<meta name="Keywords" content="Miranda Nightly, Nightly, Alpha, Nightles, Miranda Alpha, Next Miranda Version, Miranda IM alpha, Miranda Alpha plugins" />
		<meta name="Description" content="Contains the next version of Miranda IM, built from the latest CVS snapshot, generated for testing and devel usage." />
		<style type="text/css">
			body {
				padding: 0px 0px 0px 0px;
				margin: 40px 40px 40px 40px;
				background-color: #0066CC;
			}
			#content {
				background-color: #ffffff;
				margin: 0px 0px 0px 0px;
				padding: 0px 0px 0px 0px;
				border-width: 10px;
				border-style: solid;
				border-color: #003366;				
			}
			#banner {
				background-color: #f0f0f0;
				padding: 10px 10px 10px 10px;
				margin: 0px 0px 0px 0px;
				border-top-width: 0px;
				border-left-width: 0px;
				border-right-width: 0px;
				border-bottom-width: 2px;
				border-style: solid;
				border-color: #e0e0e0;
				font-family: Verdana, Arial;
				letter-spacing: 6px;
				whitespace: nowrap;
				text-align: right;
				font-variant: small-caps;
			}
			#inner-content {
				padding: 10px 10px 10px 10px;
				margin: 0px 0px 0px 0px;
				background-color: #ffffff;
				font-family: georgia, Arial;				
			}
			.section {				
				margin: 0px 0px 0px 0px;
				padding: 4px 4px 0px 4px;				
				text-align: right;
				font-family: verdana;
				font-variant: small-caps;
				font-weight: bold;
			}
			.sectiondata {
				margin: 0px 0px 0px 0px;
				padding: 0px 5px 0px 5px;
				background-color: #f5f5f5;
				border-color: #e1e1e1;
				border-width: 2px;
				border-style: solid;				
			}
			table, td { 				
				border-spacing: 0px;
				border-style: solid;
				border-width: 1px;
				border-color: #C6DBF0;
				padding: 5px;
			}
			thead {
				background-color: #C6DBF0;
				border-color: red;
				border-width: 5px;
				border-style: solid;
			}
			.filelist {	
				margin: 0px 0px 10px 0px;
				padding: 0px 0px 0px 0px;				
				width: 100%;
				border-width: 0px;
			}
			.filelist#changelog {
				margin: 0px 0px 0px 0px;
				padding: 0px 0px 5px 0px;
				font-family: courier new; 
				/* white-space: pre; */
				font-size: x-small;
			}
			.size {
				width: 70px;				
			}
			.comment {
				width: 55%;
			}
		</style>
	</head>
	<body>
		<div id="content">
			<div id="banner">
			Miranda IM: Alpha Builds
			</div>
			<div id="inner-content">
<!-- content begins -->

<div class="section">What is all this about?</div>
	<div class="sectiondata">
<p>Miranda IM is released in two ways - every few months a 'stable' version is 
<a href="http://miranda-im.org/release">released</a> this is what most people use. 
However development is faster than every few months and certain people want to be able to benefit
from new features and fixes for problems et al, quicker.
</p>

<p>
These faster releases are called <strong>alpha builds</strong> or development/CVS snapshots. 
This version of Miranda contains all the changes that have occured recently. These changes might be 
<strong>good or bad</strong>.

Unless you require a certain feature or fix which you know is contained within the latest 
development then it is recommended that you stay with <a href="http://miranda-im.org/release">release builds</a>
</p>
	</div>
	
<div class="section">File List</div>
	<div class="sectiondata">
<p>
Miranda is made up of a core application and <strong>optional</strong> components, but without
them Miranda wouldn't do much, components provide various features but some of these features
may not be required by you - it is still recommended that you download all components that are
marked in bold plus any which you require. <strong>Do not use these components with the stable
release</strong>.
</p>
<p>
<?php	
	$datestr = date('l \t\h\e jS \o\f F, Y', $timestamp); //l jS, F Y T
	echo sprintf('There have been %u change%s since the last build. The current build was generated on <strong>%s</strong> at %s %s.', sizeof($CHANGES), (sizeof($CHANGES) == 1 ? '' : 's'),
		$datestr,  date('H:i:s', $timestamp), $timezone);	
?>
</p>
<table class="filelist">
	<thead><tr>
		<td>Name</td>
		<td>Archive</td>
		<td class="size">Size</td>
		<td>Purpose</td>
	</tr></thead>
	<tbody>
	<?php
		echo get_file_list('');
	?>
	</tbody>
</table> 

	</div>
	
<div class="section">What's changed?</div>
	<div class="sectiondata">
<p>

</p>
<table class="filelist" id="changelog">
	<thead>
		<tr>
			<td>dev</td>
			<td class="comment">comment</td>
			<td>when</td>
			<td>where</td>
		</tr>
	</thead>
	<tbody>
<?php
	foreach ($CHANGES as $change) {
		$author = $change['AUTHOR'][0];
		$msg = htmlspecialchars( $change['MSG'] );
		$when = $change['DATE'][0] . ' ' . $change['TIME'][0];
		$where = '';
		foreach( $change['NAME'] as $file ) $where .= $file . ' '; 
		$where = htmlspecialchars($where);
		echo sprintf('<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> </tr>', 
			$author, $msg, $when, $where);
	}
?>
	</tbody>
</table>
	</div>
	
<div class="section">File List (Debug Version)</div>
	<div class="sectiondata">
<p>
Unless you are directed to download an alpha which contains debugging information, referred to as a <strong>debug version</strong>
there is <strong>no</strong> reason to obtain the following packages - these files contain extra information which help developers locate
where errors are occuring.
</p>

<table class="filelist">
	<thead><tr>
		<td>Name</td>
		<td>Archive</td>
		<td class="size">Size</td>
		<td>Purpose</td>
	</tr></thead>
	<tbody>
	<?php
		echo get_file_list('debug');
	?>
	</tbody>
</table> 
	</div>
	
<div class="section">CVS snapshot</div>
	<div class="sectiondata">
<p>
All developers who are associated with the project commit changes to a single repository which is only
available to them, there is a second repository which allows anonymous access to the source - Sadly this second
repository is <strong>six hours</strong> behind the <i>real</i> source tree.
</p>
<p>
Here you can download the source code checked out of the CVS that was used to build all the files
on this page, it contains a copy of the entire source tree.
</p>
<table class="filelist">
	<thead><tr>
		<td>Name</td>
		<td>Archive</td>
		<td class="size">Size</td>
		<td>Purpose</td>
	</tr></thead>
	<tbody>
		<tr>
			<td>CVS</td>
			<?php
				$fn = 'miranda_cvs_' . $buildstamp . '.zip';
				$size = @filesize($fn);
				echo sprintf('<td><a href="%s">%s</a></td>', $fn, $fn);
				echo sprintf('<td>%d KB</td>', $size ? $size / 1024 : 0 );
			?>			
			<td>Allow access to the CVS faster than the anonymous repository</td>
		</tr>
	</tbody>
</table> 
	</div>
	
<div class="section">Other bits and pieces</div>
	<div class="sectiondata">
<p>
The CVS source code is checked out periodically on a server via a cron job, this server is located in the timezone
of <?php echo $timezone ?> all check out times are based on this timezone. The build numbers of the packages 
are made up of <i>year, month, day</i> with <i>hour, minute</i> appended to allow more than one build per 24 hours, these values are derived in the timezone of <?php echo $timezone ?> as well.
</p>
<p>
To make it easier to fetch packages without explicitly knowing the exact date and time of the build, the following
symlinks (shortcuts) are kept valid, they will always point to the latest version.
</p>
<table class="filelist">
	<thead><tr>
		<td>Name</td>
		<td>Contains</td>
	</tr></thead>
	<tbody>
<?php
foreach($FILELIST as $file) {
	echo sprintf("<tr> <td> <a href=\"%s_latest.zip\">%s_latest.zip</a> </td> <td>points to the latest %s</td> </tr>",
		$file[0], $file[0], $file[0]);
}
?>
		<tr>
			<td> <a href="miranda_cvs_latest.zip">miranda_cvs_latest.zip</a> </td>
			<td>points to the latest CVS</td>
		</tr>
		<tr>
			<td> <a href="checkout.when">checkout.when</a> </td>
			<td>A text file which contains the tabulated ISO date, time and timezone of the current build</td>
		</tr>
	</tbody>
</table>
	</div>
	
<!-- content ends-->
			</div>
		</div>
	</body>
</html>