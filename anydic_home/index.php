<?php
// string ��� �����.
/*
$aa = <<< END
side by side
step by step
stem cell
END;
$wlist = explode('\n', $aa);
// �̹���� ����.
*/

$wlist = array(
"side by side",
"anticipate",
"scrutiny",
"crab",
"step by step",
"stem cell",
"take over",
"get rid of",
"crystal clear",
"crystal",
"as of",
"strike up",
"pay-as-you-go",
"crystal clear"

);


$imglist = array (
'<img src="/img/logo_hp.jpg" height=40 border=0 valign=middle>',
'<img src="/img/logo40.JPG" height=40 border=0 valign=middle>',
'<img src="/img/anydict-yahoostyle2.bmp" height=30 border=0 valign=middle>',
'<img src="/img/purpl2red.JPG" height=50 border=0 valign=middle>',
'<img src="/img/c7.jpg" height=40 border=0 valign=middle>',
'<img src="/img/han3_1.JPG" height=40 border=0 valign=middle>',
'<img src="/img/logo9-70.jpg" height=40 border=0 valign=middle>',
'<img src="/img/logo12-70.jpg" height=40 border=0 valign=middle>',
'<img src="/img/logo12-70.jpg" height=40 border=0 valign=middle>',
'<img src="/img/logo12-70.jpg" height=40 border=0 valign=middle>',
'<img src="/img/logo12-70.jpg" height=40 border=0 valign=middle>'
);



$rand_num = mt_rand(0, count($wlist)-1 );
$picked = $wlist[$rand_num];

$rand_num = mt_rand(0, count($imglist)-1 );
$imgfile = $imglist[$rand_num];
$imgfile = '<a href="/logos.html"> ' .  $imgfile . ' </a>';

//print "\n". $rand_num . " ,  " . $picked . "\n";

#print gettype($aa[$rand_num]);

/*
// string ��� �����.
// �ణ ������ ���. array_push() �̿�
$wlist = array();

array_push($wlist, "side by side");
array_push($wlist, "step by step");
array_push($wlist, "stem cell");

$rand_num = mt_rand(0, count($wlist)-1 );
*/

?>


<html>
<head>
<meta http-equiv=Content-Type content="text/html; charset=euc-kr">

<META HTTP-EQUIV="Keywords"
CONTENT="����, �н���, ����, ���, �峲��, 
 ���� ���丮, pooh, ������, ����, �ִϵ�, ����,����, ����,  �н�, ����, ����,  ����, ����, ����,
 �п�,����,������, ���� ����, ���� DVD, ��Ȱ ����, ��ȭ, ����, ���н�,
 ������, ����, ��ġ��, �ʵ�, �ߵ�, phonics,�б�, ���, ����,  DVD, ��ȭ, PDA, ����,
 AnyDict, AnyDic">


<title>�ִϵ� ���� ���������: AnyDic, AnyDict</title>

<style><!--
body,td,a,p,.h{font-family:����,����,arial,sans-serif;line-height=110%;}
.ko{font-size: 10pt;}
.h{font-size: 20px;} .h{color:} .q{text-decoration:none; color:#0000cc;}
.g{text-decoration:none; color:#0000cc;}
//-->
</style>

<SCRIPT LANGUAGE="JavaScript"> 
<!--
function fo() { 
document.adform.W.select(); 
document.adform.W.focus(); 
}

function open_daum() {
daumwin = window.open('', "daumwin", "scrollbars=yes,toolbar=no,location=no, directories=no,width=650,height=550,resizable=yes,mebar=no");
}
-->
</script> 
</head>

<body bgcolor=#ffffff text=#000000 link=#0000cc vlink=#0000cc alink=#ff0000 onLoad="fo()">

<center>
<TABLE cellSpacing=0 cellPadding=0 width="100%" border=0>
<TBODY>
<TR align="center">
<TD noWrap align="center" width=20%> <a href="/logos.html"><IMG alt="�ִϵ� �ΰ� �� �� ..." src="/img/logo8-70.jpg" border=0></a> </TD>   
</TR>

<TR align="center">
<TD height=100% align="center" noWrap ><FONT color=green><B>�ִϵ� ���� ���������</B></FONT> </td> 
</TR>
</TBODY></TABLE>

</center>
<br/> 

<center>
<TABLE cellSpacing=0 cellPadding=0 width="728" bgColor=#00b4da border=0>
<TBODY>
<TR height=49>
<TD>
  <TABLE cellSpacing=0 cellPadding=0 border=0>
	<form name="adform" action="/ad.dic">
	<TBODY>
	<TR height=2> <TD colSpan=4></TD></TR>
	<TR>
	  <TD class=base vAlign=middle noWrap rowSpan=3>
	   <FONT color=#ffffff>&nbsp;<B>����/����/����/���� </B></FONT>&nbsp;&nbsp;</TD>
	  <TD style="PADDING-TOP: 8px">
	   <input type=hidden name="F" value=3>

<?
   print '<INPUT size=30 name="W"  maxlength=50 value=';
	print '"'.$picked.'"';		
	print '> </TD> ';
?>

	  <TD style="PADDING-TOP: 8px">
		<input name="" type=image height=21 alt=�˻� hspace=5 src="img/srch2.gif" width=35 align=absMiddle border=0></TD>

	  <TD class=base style="COLOR: #ffffff; PADDING-TOP: 8px" 
		noWrap>&nbsp;&nbsp;&nbsp;<A href="/help.html">
		<FONT color=#ffffff>����</FONT></A> </TD>
	</TR>

	<TR height=2> <TD colSpan=4></TD></TR>
	 </FORM></TBODY></TABLE>
</TD>
</TR>
</TBODY></TABLE>


<br/> <br/>
<font size=-1>
�ܾ��� �ǹ̸�  ǥ���ϴ� �׸� ����Ÿ�� �����˴ϴ�. (2,000 ���� �ܾ�)<br/>
</font>
<font size=-1> ���� ���� ����Ʈ���� ���� ������ �������� �����ǰ� �ֽ��ϴ�. </font>

<br/> <br/>
<br/>

<center>
<script type="text/javascript"><!--
google_ad_client = "pub-3002816070890467";
google_ad_width = 728;
google_ad_height = 90;
google_ad_format = "728x90_as";
google_ad_type = "text_image";
google_ad_channel ="";
google_color_border = "DFF2FD";
google_color_bg = "DFF2FD";
google_color_link = "03364C3";
google_color_url = "008000";
google_color_text = "000000";
//--></script>
<script type="text/javascript"
  src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>
</center>


<center>
<br/> 
<hr size=1> 

<?
print $imgfile;
?>
<br/> <br/>

 <font size=-1>&copy;2002,2006 
&nbsp; &nbsp; 

<font size=-1>  <a href="/comment.html">Any comment on Anydict </a> </font> 
&nbsp; &nbsp;

<a href="http://www.dnsever.com" target="dnsever">
powered by DNSever
</a>

</center>

</body>
</html>
