$(document).ready(function(){
    $('a').filter(".bibref").hover(function(e){
        $(this).attr('title', '');
    });
});
