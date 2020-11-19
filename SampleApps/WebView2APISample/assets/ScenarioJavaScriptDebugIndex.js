function onAddClick() {
    console.log("onAddClick+");
    const list = document.getElementById('list');
    const index = (list.childElementCount + 1);
    const x = 0;
    const y = 0;

    const item = document.createElement('div');
    item.classList.add('item');
    item.classList.add('container');
    const title = document.createElement('div');
    title.classList.add('title');
    title.innerText = 'Item ' + index;
    item.appendChild(title);
    const content = document.createElement('div');
    content.classList.add('content');

    if (index % 2 === 0) {
        content.style.backgroundImage =
        `url(https://edgetipscdn.microsoft.com/insider-site/images/logo-dev.c8d75c3b.png)`;
    } else {
        content.style.backgroundImage =
        `url(https://edgetipscdn.microsoft.com/insider-site/images/logo-canary.a897af1f.png)`;
    }

    item.appendChild(content);
    list.appendChild(item);
    console.log("onAddClick-");
}

console.log("inside");
// Add the event handler as soon as the script loads
const elem = document.getElementById('addButton');
elem.addEventListener('click', onAddClick);

// Fill in some items for testing
for (let i = 0; i < 2; i++) {
    onAddClick();
}
