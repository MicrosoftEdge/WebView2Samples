function onHeaderClick() {
    console.log("onHeaderClick+");
    const header = document.getElementById('header');
    document.getElementById('HeaderSpace').textContent = header.textContent;
    console.log("onHeaderClick-");
}

console.log("inside");
// Add the event handler as soon as the script loads
const elem = document.getElementById('getHeader');
elem.addEventListener('click', onHeaderClick);
