<!-- 
Documentation Overview:
This README serves as the main page of the P4 compiler documentation.

It provides an entry point to the various sections, guides, and references available for the P4 compiler.

Direct link to documentation: [P4 Compiler Documentation](https://p4lang.github.io/p4c/)
-->

@htmlonly
<div class="diagram-container">
<!-- draw.io diagram -->
<iframe id="svgFrame" src="architecture_unanimated.html" width="100%" height="500px" style="border:none;"></iframe>
</div>

<button id="toggle" onclick="animateFlow()">Toggle</button>

<script>
function animateFlow() {
    const iframe = document.getElementById('svgFrame');
    const svgDocument = iframe.contentDocument;
    if (svgDocument) {
        const edges = svgDocument.querySelectorAll('path:not([marker-end])');
        edges.forEach(edge => {
            edge.classList.toggle('flow');
        });
    const toggleButton = document.getElementById('toggle');
    if (toggleButton) {
             toggleButton.style.display = 'none';
                 }
    }
}
</script>
@endhtmlonly

<div class="content-spacing"></div>
<div class="content-spacing"></div>
[P4 (Programming Protocol-independent Packet Processors)](https://p4.org/) is a language for expressing how packets are processed by the data-plane of a programmable network element, e.g.(hardware or software switch, Smart-NIC, and network function appliance).
<div class="content-spacing"></div>
P4C is the official open-source reference compiler for the [P4](https://p4.org/) programming language, supporting both P4-14 and P4-16.
<div class="content-spacing"></div>
<div class="content-spacing"></div> 

## Features of P4C

- Provides compatibility for all versions of %P4.
- Open-source compiler front end, allows anyone to quickly build a compiler for a new architecture.
- Supports multiple back-ends for application specific integrated circuits (ASICs), network-interface cards (NICs), field-programmable gate arrays (FPGAs), software switches and other targets.
- Promotes the expansion of software development tools like debuggers, integrated development environments, %P4 control-planes, testing and formal verification tools.
- The compiler has an extensible architecture, making it easy to add new passes and optimizations, and to hook up new back-ends. Furthermore, the front-end and mid-end can be further detailed and extended by target-specific back-end. 

@htmlonly
<!-- https://www.svgrepo.com/collection/scarlab-oval-line-icons/ -->
<div class="card-container">

  <div class="card-item">
    <a href="https://p4lang.github.io/p4c/getting_started.html#p4-compiler-onboarding"> 
    <div class="card-content">
   <?xml version="1.0" encoding="utf-8"?>
      <svg width="800px" height="800px" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M4 19V6.2C4 5.0799 4 4.51984 4.21799 4.09202C4.40973 3.71569 4.71569 3.40973 5.09202 3.21799C5.51984 3 6.0799 3 7.2 3H16.8C17.9201 3 18.4802 3 18.908 3.21799C19.2843 3.40973 19.5903 3.71569 19.782 4.09202C20 4.51984 20 5.0799 20 6.2V17H6C4.89543 17 4 17.8954 4 19ZM4 19C4 20.1046 4.89543 21 6 21H20M9 7H15M9 11H15M19 17V21" stroke="#000000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>
      <h2>P4 Compiler Onboarding</h2>
      <p>Explore tutorials and documentation to understand how the P4 compiler works.</p>
    </div>
    </a>
  </div>

  <div class="card-item">
    <a href="https://p4lang.github.io/p4c/getting_started.html"> 
    <div class="card-content">
      <svg width="800px" height="800px" viewBox="0 0 24 24" fill="none"  class="card_svg" xmlns="http://www.w3.org/2000/svg">
        <path opacity="0.1" fill-rule="evenodd" clip-rule="evenodd" d="M12 21C16.9706 21 21 16.9706 21 12C21 7.02944 16.9706 3 12 3C7.02944 3 3 7.02944 3 12C3 16.9706 7.02944 21 12 21ZM15.224 13.0171C16.011 12.5674 16.011 11.4326 15.224 10.9829L10.7817 8.44446C10.0992 8.05446 9.25 8.54727 9.25 9.33333L9.25 14.6667C9.25 15.4527 10.0992 15.9455 10.7817 15.5555L15.224 13.0171Z" fill="#323232"/>
        <path d="M21 12C21 16.9706 16.9706 21 12 21C7.02944 21 3 16.9706 3 12C3 7.02944 7.02944 3 12 3C16.9706 3 21 7.02944 21 12Z" stroke="#323232" stroke-width="1"/>
        <path d="M10.9 8.8L10.6577 8.66152C10.1418 8.36676 9.5 8.73922 9.5 9.33333L9.5 14.6667C9.5 15.2608 10.1418 15.6332 10.6577 15.3385L10.9 15.2L15.1 12.8C15.719 12.4463 15.719 11.5537 15.1 11.2L10.9 8.8Z" stroke="#323232" stroke-width="1" stroke-linecap="round" stroke-linejoin="round"/></svg>
      <h2>Getting Started</h2>
      <p>Set up the <b>P4 compiler</b> and write your first P4 program.</p>
    </div>
    </a>
  </div>

  <div class="card-item">
    <a href="https://p4lang.github.io/p4c/contribute.html"> 
    <div class="card-content">
      <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><g id="SVGRepo_bgCarrier" stroke-width="0"></g><g id="SVGRepo_tracerCarrier" stroke-linecap="round" stroke-linejoin="round"></g><g id="SVGRepo_iconCarrier"> <path d="M3 9.5C3 9.03534 3 8.80302 3.03843 8.60982C3.19624 7.81644 3.81644 7.19624 4.60982 7.03843C4.80302 7 5.03534 7 5.5 7H12H18.5C18.9647 7 19.197 7 19.3902 7.03843C20.1836 7.19624 20.8038 7.81644 20.9616 8.60982C21 8.80302 21 9.03534 21 9.5V9.5V9.5C21 9.96466 21 10.197 20.9616 10.3902C20.8038 11.1836 20.1836 11.8038 19.3902 11.9616C19.197 12 18.9647 12 18.5 12H12H5.5C5.03534 12 4.80302 12 4.60982 11.9616C3.81644 11.8038 3.19624 11.1836 3.03843 10.3902C3 10.197 3 9.96466 3 9.5V9.5V9.5Z" stroke="#000000" stroke-width="2" stroke-linejoin="round"></path> <path d="M4 12V16C4 17.8856 4 18.8284 4.58579 19.4142C5.17157 20 6.11438 20 8 20H9H15H16C17.8856 20 18.8284 20 19.4142 19.4142C20 18.8284 20 17.8856 20 16V12" stroke="#000000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M12 7V20" stroke="#000000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M11.3753 6.21913L9.3959 3.74487C8.65125 2.81406 7.26102 2.73898 6.41813 3.58187C5.1582 4.8418 6.04662 7 7.82843 7L11 7C11.403 7 11.6271 6.53383 11.3753 6.21913Z" stroke="#000000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path> <path d="M12.6247 6.21913L14.6041 3.74487C15.3488 2.81406 16.739 2.73898 17.5819 3.58187C18.8418 4.8418 17.9534 7 16.1716 7L13 7C12.597 7 12.3729 6.53383 12.6247 6.21913Z" stroke="#000000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path> </g></svg>
      <h2>Contribute</h2>
      <p>Join our community! Learn how you can help improve the <b>P4 compiler</b> and work with others.</p>
    </div>
    </a>
  </div>
</div>

@endhtmlonly
