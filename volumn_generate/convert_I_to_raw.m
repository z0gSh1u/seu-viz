N = size(I);
N = N(1);

fid = fopen(strcat('shep3d_', num2str(N), '.raw'), 'wb');
fwrite(fid, I, 'float');
fclose(fid);

disp('Done.');